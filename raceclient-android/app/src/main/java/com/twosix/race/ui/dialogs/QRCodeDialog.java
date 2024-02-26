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
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;

import androidx.appcompat.app.AlertDialog;

import ShimsJava.RaceHandle;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.qrcode.QRCodeWriter;
import com.twosix.race.R;

public class QRCodeDialog extends RaceDialogFragment {

    private static final String TAG = "QRCodeDialog";

    private static final String ARG_ALERT_HANDLE = "alertHandle";
    private static final String ARG_QR_DATA = "qrData";

    public static QRCodeDialog newInstance(RaceHandle handle, String qrData) {
        Log.v(TAG, "newInstance");

        Bundle args = new Bundle();
        args.putLong(ARG_ALERT_HANDLE, handle.getValue());
        args.putString(ARG_QR_DATA, qrData);

        QRCodeDialog dialog = new QRCodeDialog();
        dialog.setArguments(args);

        return dialog;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Log.v(TAG, "onCreateDialog");

        Bundle args = getArguments();
        RaceHandle handle = new RaceHandle(args.getLong(ARG_ALERT_HANDLE));
        String qrData = args.getString(ARG_QR_DATA);

        LayoutInflater factory = LayoutInflater.from(this.getContext());
        final View qrCodeView = factory.inflate(R.layout.fragment_qr_code, null);
        ImageView imageView = qrCodeView.findViewById(R.id.qr_imageview);

        QRCodeWriter qrCodeWriter = new QRCodeWriter();
        try {
            BitMatrix bitMatrix = qrCodeWriter.encode(qrData, BarcodeFormat.QR_CODE, 200, 200);
            Bitmap bitmap = Bitmap.createBitmap(200, 200, Bitmap.Config.RGB_565);
            for (int x = 0; x < 200; x++) {
                for (int y = 0; y < 200; y++) {
                    bitmap.setPixel(x, y, bitMatrix.get(x, y) ? Color.BLACK : Color.WHITE);
                }
            }
            imageView.setImageBitmap(bitmap);
        } catch (Exception err) {
            Log.e(TAG, "Error creating QR code image", err);
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setMessage(qrData)
                .setCancelable(false)
                .setPositiveButton(
                        android.R.string.ok,
                        (dialog, which) -> {
                            new Thread(() -> raceService.acknowledgeAlert(handle)).start();
                        })
                .setView(qrCodeView);
        return builder.create();
    }
}
