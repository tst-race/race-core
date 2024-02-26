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

package main

import (
	"errors"
	commsShims "shims"
)

const (
	DIRECT_CHANNEL_GID   = "twoSixDirectGolang"
	INDIRECT_CHANNEL_GID = "twoSixIndirectGolang"
	_INT_MAX             = 2147483647
)

func getDefaultChannelPropertiesForChannel(sdk commsShims.IRaceSdkComms, channelGid string) commsShims.ChannelProperties {
	return sdk.GetChannelProperties(channelGid)
}

func getDefaultLinkPropertiesForChannel(sdk commsShims.IRaceSdkComms, channelGid string) (commsShims.LinkProperties, error) {
	props := commsShims.NewLinkProperties()
	if channelGid == DIRECT_CHANNEL_GID {
		channelProps := getDefaultChannelPropertiesForChannel(sdk, channelGid)

		props.SetTransmissionType(channelProps.GetTransmissionType())
		props.SetConnectionType(channelProps.GetConnectionType())
		props.SetSendType(channelProps.GetSendType())
		props.SetReliable(channelProps.GetReliable())
		props.SetIsFlushable(channelProps.GetIsFlushable())
		props.SetDuration_s(channelProps.GetDuration_s())
		props.SetPeriod_s(channelProps.GetPeriod_s())
		props.SetMtu(channelProps.GetMtu())

		worstLinkPropertySet := commsShims.NewLinkPropertySet()
		worstLinkPropertySet.SetBandwidth_bps(23130000)
		worstLinkPropertySet.SetLatency_ms(17)
		worstLinkPropertySet.SetLoss(0.1)
		props.GetWorst().SetSend(worstLinkPropertySet)
		props.GetWorst().SetReceive(worstLinkPropertySet)

		props.SetExpected(channelProps.GetCreatorExpected())

		bestLinkPropertySet := commsShims.NewLinkPropertySet()

		bestLinkPropertySet.SetBandwidth_bps(28270000)
		bestLinkPropertySet.SetLatency_ms(14)
		bestLinkPropertySet.SetLoss(0.1)
		props.GetBest().SetSend(bestLinkPropertySet)
		props.GetBest().SetReceive(bestLinkPropertySet)

		props.SetSupported_hints(channelProps.GetSupported_hints())
		props.SetChannelGid(channelGid)

		return props, nil
	} else if channelGid == INDIRECT_CHANNEL_GID {
		channelProps := getDefaultChannelPropertiesForChannel(sdk, channelGid)

		props.SetLinkType(commsShims.LT_BIDI)
		props.SetTransmissionType(channelProps.GetTransmissionType())
		props.SetConnectionType(channelProps.GetConnectionType())
		props.SetSendType(channelProps.GetSendType())
		props.SetReliable(channelProps.GetReliable())
		props.SetIsFlushable(channelProps.GetIsFlushable())
		props.SetDuration_s(channelProps.GetDuration_s())
		props.SetPeriod_s(channelProps.GetPeriod_s())
		props.SetMtu(channelProps.GetMtu())

		worstLinkPropertySet := commsShims.NewLinkPropertySet()
		worstLinkPropertySet.SetBandwidth_bps(277200)
		worstLinkPropertySet.SetLatency_ms(3190)
		worstLinkPropertySet.SetLoss(0.1)
		props.GetWorst().SetSend(worstLinkPropertySet)
		props.GetWorst().SetReceive(worstLinkPropertySet)

		props.SetExpected(channelProps.GetCreatorExpected())

		bestLinkPropertySet := commsShims.NewLinkPropertySet()

		bestLinkPropertySet.SetBandwidth_bps(338800)
		bestLinkPropertySet.SetLatency_ms(2610)
		bestLinkPropertySet.SetLoss(0.1)
		props.GetBest().SetSend(bestLinkPropertySet)
		props.GetBest().SetReceive(bestLinkPropertySet)

		props.SetSupported_hints(channelProps.GetSupported_hints())
		props.SetChannelGid(channelGid)

		return props, nil
	}

	return props, errors.New("invalid channel GID")
}
