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

import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import ShimsJava.RaceHandle;

import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import com.twosix.race.R;

public class InputDialog extends RaceDialogFragment {

    private static final String TAG = "InputDialog";

    private static final String ARG_INPUT_HANDLE = "inputHandle";
    private static final String ARG_INPUT_PROMPT = "inputPrompt";

    public static InputDialog newInstance(RaceHandle handle, String prompt) {
        Log.v(TAG, "newInstance");

        Bundle args = new Bundle();
        args.putLong(ARG_INPUT_HANDLE, handle.getValue());
        args.putString(ARG_INPUT_PROMPT, prompt);

        InputDialog dialog = new InputDialog();
        dialog.setArguments(args);

        return dialog;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState) {
        Log.v(TAG, "onCreateDialog");

        Bundle args = getArguments();
        RaceHandle handle = new RaceHandle(args.getLong(ARG_INPUT_HANDLE));
        String prompt = args.getString(ARG_INPUT_PROMPT);

        View view = LayoutInflater.from(getActivity()).inflate(R.layout.fragment_text_input, null);
        TextInputLayout textInputLayout = view.findViewById(R.id.text_input_layout);
        textInputLayout.setHint("");

        final TextInputEditText textInput = view.findViewById(R.id.text_input);

        AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(getActivity());
        alertDialogBuilder
                .setMessage(prompt)
                .setView(view)
                .setCancelable(false)
                .setPositiveButton(
                        android.R.string.ok,
                        (dialog, which) -> {
                            String response = textInput.getText().toString();
                            new Thread(
                                            () ->
                                                    raceService.processUserInputResponse(
                                                            handle, true, response))
                                    .start();
                            dialog.dismiss();
                        })
                .setNegativeButton(
                        android.R.string.cancel,
                        (dialog, which) -> {
                            new Thread(
                                            () ->
                                                    raceService.processUserInputResponse(
                                                            handle, false, ""))
                                    .start();
                        });
        return alertDialogBuilder.create();
    }
}
