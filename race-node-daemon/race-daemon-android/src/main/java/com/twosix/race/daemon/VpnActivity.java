//
// Copyright 2023 Two Six Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package com.twosix.race.daemon;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;

import androidx.appcompat.app.AppCompatActivity;

import de.blinkt.openvpn.api.IOpenVPNAPIService;
import de.blinkt.openvpn.api.IOpenVPNStatusCallback;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/** Activity to interact with the OpenVPN service. */
public class VpnActivity extends AppCompatActivity {

    private final Logger logger = LoggerFactory.getLogger(getClass());

    private static final String CONNECT_VPN_ACTION = "com.twosix.race.daemon.CONNECT_VPN";
    private static final String DISCONNECT_VPN_ACTION = "com.twosix.race.daemon.DISCONNECT_VPN";

    private static final int OPENVPN_PERMISSION_REQUEST = 1;

    private ScheduledExecutorService executor = Executors.newSingleThreadScheduledExecutor();
    private ScheduledFuture<?> dnsTestFuture = null;
    private ScheduledFuture<?> connectionTimeoutFuture = null;

    private Intent intent = null;
    private IOpenVPNAPIService vpnService = null;
    private IOpenVPNStatusCallback vpnCallback = null;

    /** Service connection for the OpenVPN API service. */
    private ServiceConnection serviceConnection =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    vpnService = IOpenVPNAPIService.Stub.asInterface(service);
                    try {
                        // OpenVPN has its own permissions scheme where the user has to confirm
                        // that an app can control OpenVPN. If the app has already been authorized,
                        // this will be null. Otherwise the intent will launch the permission
                        // request
                        // activity.
                        Intent intent = vpnService.prepare(getPackageName());
                        if (intent != null) {
                            logger.info("Requesting permission for OpenVPN service");
                            startActivityForResult(intent, OPENVPN_PERMISSION_REQUEST);
                        } else {
                            onActivityResult(OPENVPN_PERMISSION_REQUEST, RESULT_OK, null);
                        }
                    } catch (RemoteException err) {
                        logger.error("Unable to setup permissions for VPN", err);
                        finish();
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    if (vpnService != null && vpnCallback != null) {
                        try {
                            vpnService.unregisterStatusCallback(vpnCallback);
                        } catch (RemoteException err) {
                            logger.error("Unable to unregister status callback", err);
                        }
                    }
                    vpnService = null;
                }
            };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        intent = getIntent();
        bindVpnService();
    }

    /** Binds the activity to the OpenVPN API service. */
    private void bindVpnService() {
        Intent intent = new Intent(IOpenVPNAPIService.class.getName());
        intent.setPackage("de.blinkt.openvpn");

        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    /**
     * If permission for the OpenVPN service was granted, performs the action described by the
     * original intent.
     *
     * @param requestCode Identifies the request for which the result applies
     * @param resultCode Result for the request
     * @param data Not used
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == RESULT_OK) {
            if (requestCode == OPENVPN_PERMISSION_REQUEST) {
                vpnCallback =
                        new IOpenVPNStatusCallback.Stub() {
                            @Override
                            public void newStatus(
                                    String uuid, String state, String message, String level)
                                    throws RemoteException {
                                handleVpnStatus(uuid, state, message, level);
                            }
                        };
                try {
                    vpnService.registerStatusCallback(vpnCallback);
                } catch (RemoteException err) {

                }

                if (intent.getAction().equals(CONNECT_VPN_ACTION)) {
                    connectVpnProfile();
                } else if (intent.getAction().equals(DISCONNECT_VPN_ACTION)) {
                    disconnectVpn();
                } else {
                    logger.warn("Unrecognized action: " + intent.getAction());
                }
            }

            // Schedule the activity clean up just in case we never end up connecting
            connectionTimeoutFuture = executor.schedule(this::cleanup, 60, TimeUnit.SECONDS);
        } else {
            logger.error("Unable to obtain permissions for VPN service");
            finish();
        }
    }

    /**
     * Respond to VPN status callbacks. If the VPN has finished connecting as a response to a
     * connection attempt, or if the VPN has finished disconnecting as a response to a disconnection
     * attempt, we can finish this activity.
     *
     * @param uuid Associated VPN profile UUID
     * @param state VPN state
     * @param message Status message
     * @param level VPN operational level
     */
    private void handleVpnStatus(String uuid, String state, String message, String level) {
        logger.debug(
                "Received VPN status callback: "
                        + uuid
                        + ", "
                        + state
                        + ", "
                        + message
                        + ", "
                        + level);
        if (intent.getAction().equals(CONNECT_VPN_ACTION)) {
            if (state.equals("CONNECTED")) {
                logger.info("VPN successfully connected");
                // Wait a little bit and then confirm that DNS is properly configured
                // (this delay is arbitrary but seemed to provide more stability than immediately
                // performing the DNS lookup test, as well as frees up the VPN service thread)
                dnsTestFuture = executor.schedule(this::testDnsLookup, 2, TimeUnit.SECONDS);
            }
        } else if (intent.getAction().equals(DISCONNECT_VPN_ACTION)) {
            if (state.equals("NOPROCESS")) {
                logger.info("VPN successfully disconnected");
                cleanup();
            }
        }
    }

    private void testDnsLookup() {
        try {
            logger.info("Testing DNS lookup");
            InetAddress.getByName("rib-file-server");
            logger.info("DNS lookup succeeded");
            cleanup();
        } catch (UnknownHostException err) {
            logger.warn("DNS lookup failed, will retry connection attempt: " + err.getMessage());
            connectVpnProfile();
        }
    }

    /**
     * Cleanup and finish this activity. If there is a timeout cleanup task scheduled, cancel it
     * first.
     */
    private void cleanup() {
        if (dnsTestFuture != null) {
            dnsTestFuture.cancel(true);
            dnsTestFuture = null;
        }
        if (connectionTimeoutFuture != null) {
            connectionTimeoutFuture.cancel(true);
            connectionTimeoutFuture = null;
        }
        unbindService(serviceConnection);
        finish();
    }

    /**
     * Instructs the OpenVPN API service to add then start the VPN profile defined in the file
     * identified in the original intent.
     */
    private void connectVpnProfile() {
        String vpnConfigFile = intent.getStringExtra("vpn-profile-file");
        if (vpnConfigFile == null || vpnConfigFile.isEmpty()) {
            logger.error("No VPN profile configuration file provided, unable to connect VPN");
            return;
        }

        String vpnConfig = null;
        try {
            vpnConfig = new String(Files.readAllBytes(Paths.get(vpnConfigFile)));
        } catch (IOException err) {
            logger.error("Unable to read VPN profile configuration file", err);
            return;
        }
        if (vpnConfig == null || vpnConfig.isEmpty()) {
            logger.error("No VPN profile configuration found in file, unable to connect VPN");
            return;
        }

        try {
            vpnService.startVPN(vpnConfig);
            logger.info("Started VPN");
        } catch (RemoteException err) {
            logger.error("Unable to register and start VPN profile", err);
        }
    }

    /** Instructs the OpenVPN API service to disconnect the current VPN connection. */
    private void disconnectVpn() {
        try {
            vpnService.disconnect();
        } catch (RemoteException err) {
            logger.error("Unable to disconnect VPN", err);
            return;
        }
    }
}
