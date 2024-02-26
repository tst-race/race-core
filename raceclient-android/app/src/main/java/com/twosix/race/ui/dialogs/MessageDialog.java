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

package com.twosix.race.ui.dialogs;

import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;

import androidx.appcompat.app.AlertDialog;

import ShimsJava.RaceHandle;

public class MessageDialog extends RaceDialogFragment {

    private static final String TAG = "MessageDialog";

    private static final String ARG_ALERT_HANDLE = "alertHandle";
    private static final String ARG_ALERT_MESSAGE = "alertMessage";

    public static MessageDialog newInstance(RaceHandle handle, String message) {
        Log.v(TAG, "newInstance");

        Bundle args = new Bundle();
        args.putLong(ARG_ALERT_HANDLE, handle.getValue());
        args.putString(ARG_ALERT_MESSAGE, message);

        MessageDialog dialog = new MessageDialog();
        dialog.setArguments(args);

        return dialog;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Log.v(TAG, "onCreateDialog");

        Bundle args = getArguments();
        RaceHandle handle = new RaceHandle(args.getLong(ARG_ALERT_HANDLE));
        String message = args.getString(ARG_ALERT_MESSAGE);

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(message)
                .setCancelable(false)
                .setPositiveButton(
                        android.R.string.ok,
                        (dialog, which) -> {
                            new Thread(() -> raceService.acknowledgeAlert(handle)).start();
                        });
        return builder.create();
    }
}
