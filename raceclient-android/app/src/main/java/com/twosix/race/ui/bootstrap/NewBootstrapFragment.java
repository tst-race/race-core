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

package com.twosix.race.ui.bootstrap;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;

import androidx.appcompat.widget.AppCompatImageButton;
import androidx.navigation.fragment.NavHostFragment;

import ShimsJava.ChannelStatus;
import ShimsJava.ConnectionType;
import ShimsJava.JChannelProperties;

import com.google.android.material.textfield.TextInputEditText;
import com.twosix.race.R;
import com.twosix.race.ui.RaceFragment;

/** View fragment to allow the user to initiate a bootstrap operation. */
public class NewBootstrapFragment extends RaceFragment {

    private static final String TAG = "NewBootstrapFragment";

    private TextInputEditText passphraseInput;
    private RadioGroup channels;
    private View advancedOptions;
    private Spinner platformSpinner;
    private Spinner archSpinner;
    private Spinner nodeTypeSpinner;
    private Button prepareButton;

    private ArrayAdapter<CharSequence> createSpinnerAdapter(int optionsResId) {
        ArrayAdapter<CharSequence> adapter =
                ArrayAdapter.createFromResource(
                        getContext(), optionsResId, android.R.layout.simple_spinner_item);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        return adapter;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        View view = inflater.inflate(R.layout.fragment_new_bootstrap, container, false);
        passphraseInput = view.findViewById(R.id.bootstrap_passphrase);
        passphraseInput.setText("");
        passphraseInput.addTextChangedListener(
                new TextWatcher() {
                    @Override
                    public void beforeTextChanged(
                            CharSequence s, int start, int count, int after) {}

                    @Override
                    public void onTextChanged(CharSequence s, int start, int before, int count) {}

                    @Override
                    public void afterTextChanged(Editable s) {
                        validateRequiredFields();
                    }
                });

        channels = view.findViewById(R.id.bootstrap_channel_radio_group);
        channels.setOnCheckedChangeListener(this::onBootstrapChannelChanged);
        for (JChannelProperties channelProperties : raceService.getChannels(null)) {
            if (channelProperties.channelStatus.equals(ChannelStatus.CHANNEL_AVAILABLE)
                    && channelProperties.connectionType.equals(ConnectionType.CT_LOCAL)) {
                RadioButton button = new RadioButton(getContext());
                button.setText(channelProperties.channelGid);
                channels.addView(button);
            }
        }

        AppCompatImageButton expandButton = view.findViewById(R.id.expand_button);
        expandButton.setOnClickListener(this::toggleAdvancedOptions);

        advancedOptions = view.findViewById(R.id.advanced_options);
        advancedOptions.setVisibility(View.INVISIBLE);

        platformSpinner = view.findViewById(R.id.platform_spinner);
        platformSpinner.setAdapter(createSpinnerAdapter(R.array.bootstrap_platforms));
        platformSpinner.setSelection(0); // should be "android"

        archSpinner = view.findViewById(R.id.arch_spinner);
        archSpinner.setAdapter(createSpinnerAdapter(R.array.bootstrap_architectures));
        archSpinner.setSelection(0); // should be "arm64-v8a"

        nodeTypeSpinner = view.findViewById(R.id.node_type_spinner);
        nodeTypeSpinner.setAdapter(createSpinnerAdapter(R.array.bootstrap_node_types));
        nodeTypeSpinner.setSelection(0); // should be "client"

        prepareButton = view.findViewById(R.id.prepare_button);
        prepareButton.setOnClickListener(this::prepareToBootstrap);
        prepareButton.setEnabled(false);

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().setTitle(R.string.new_bootstrap_title);
    }

    private void toggleAdvancedOptions(View expandButtonView) {
        ImageButton expandButton = (ImageButton) expandButtonView;

        if (advancedOptions.getVisibility() == View.VISIBLE) {
            advancedOptions.setVisibility(View.INVISIBLE);
            expandButton.setImageResource(R.drawable.ic_expand_more);
        } else {
            advancedOptions.setVisibility(View.VISIBLE);
            expandButton.setImageResource(R.drawable.ic_expand_less);
        }
    }

    private String getPassphrase() {
        return passphraseInput.getText().toString().trim();
    }

    private String getSelectedBootstrapChannel() {
        int radioButtonId = channels.getCheckedRadioButtonId();
        if (radioButtonId != -1) {
            View radioButtonView = channels.findViewById(radioButtonId);
            if (radioButtonView != null) {
                RadioButton radioButton = (RadioButton) radioButtonView;
                return radioButton.getText().toString();
            }
        }
        return null;
    }

    private void onBootstrapChannelChanged(RadioGroup group, int checkedId) {
        validateRequiredFields();
    }

    private boolean hasAllRequiredFields() {
        String passphrase = getPassphrase();
        if (passphrase == null || passphrase.isEmpty()) {
            return false;
        }
        String bootstrapChannelId = getSelectedBootstrapChannel();
        if (bootstrapChannelId == null || bootstrapChannelId.isEmpty()) {
            return false;
        }
        return true;
    }

    private void validateRequiredFields() {
        prepareButton.setEnabled(hasAllRequiredFields());
    }

    private void prepareToBootstrap(View prepareButtonView) {
        String passphrase = getPassphrase();
        String bootstrapChannelId = getSelectedBootstrapChannel();
        String platform = platformSpinner.getSelectedItem().toString();
        String arch = archSpinner.getSelectedItem().toString();
        String nodeType = nodeTypeSpinner.getSelectedItem().toString();

        new Thread(
                        () ->
                                raceService.prepareToBootstrap(
                                        platform, arch, nodeType, passphrase, bootstrapChannelId))
                .start();

        // TODO nav to progress screen
        NavHostFragment.findNavController(this).popBackStack();
    }
}
