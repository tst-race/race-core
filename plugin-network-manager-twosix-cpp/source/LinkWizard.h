
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

#ifndef __PLUGIN_NETWORK_MANAGER_TWOSIX_LINK_WIZARD_H__
#define __PLUGIN_NETWORK_MANAGER_TWOSIX_LINK_WIZARD_H__

#include <ChannelProperties.h>
#include <ChannelRole.h>
#include <ChannelStatus.h>
#include <IRaceSdkNM.h>
#include <LinkStatus.h>
#include <LinkType.h>
#include <SdkResponse.h>
#include <opentracing/tracer.h>

#include <mutex>  // std::mutex, std::lock_guard
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Persona.h"
#include "RaceCrypto.h"

class PluginNMTwoSix;

class LinkWizard {
public:
    LinkWizard(const std::string &raceUuid, PersonaType personaType, PluginNMTwoSix *sdk);

    /**
     * @brief Inform the LinkWizard that network manager believes everything is in a good ready
     * state. Namely, that all channels are no longer in an intermediate state and trigger delayed
     * getSupportedChannels requests to be responsed to.
     *
     * @param new_ready The new status value
     */
    void setReadyToRespond(bool new_ready);

    /**
     * @brief Initialize the LinkWizard with the set of personas it will be dealing with. Triggers
     * supported channel queries to those nodes, so sendFormattedMsg to each node should succeed
     * (i.e. there should be a connection to each node already).
     *
     * @param personas vector<Persona> of the personas to initialize.
     * @return true if querySupportedChannel to each persona succeeds, false otherwise
     */
    void init();

    /**
     * @brief Re-send the list of supported channels to all known nodes, usually in response to a
     * channel being enabled or disabled.
     */
    void resendSupportedChannels();

    /**
     * @brief Add another persona for the LinkWizard to interact with. This triggers a supported
     * channel query to that node, so sendFormattedMsg should succeed (i.e. a connection to that
     * node should already exist). This function is not idempotent - each call will result in a new
     * query even for an already-added node
     *
     * @param personas The Persona to add to the LinkWizard
     * @return true if querySupportedChannel succeeds
     */
    bool addPersona(const Persona &persona);

    /**
     * @brief Get the number of requests the LinkWizard has not finished trying to fulfill
     *
     * @return the number of requests
     */
    uint32_t numOutstandingRequests();

    /**
     * @brief Handle a LinkWizard protocol message, potentially triggering creation/loading of new
     * links and/or sending messages to other personas
     *
     * @param persona The Persona that sent the message to us
     * @param extMsg The ExtClrMsg itself (contains needed opentracing info)
     *
     * @return bool Whether the parsing and attendant actions of the message happened successfully.
     */
    bool processLinkMsg(const Persona &sender, const ExtClrMsg &extMsg);

    /**
     * @brief Attempt to construct a new unicast link with the persona of the type specified. If
     * linkType is LT_BIDI this may cause creation of two unidirectional links. This returns false
     * then either there are no shared supported channels or there was an error attempting to create
     * the link or sending the message.
     *
     * @param persona The Persona to establish a unicast link with
     * @param linkType The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
     * @param channelGid The desired channel GID
     * @param linkSide The desired side (creator, loader, both/either) of the link to be obtained
     *      on this node
     * @return bool Success of the operation.
     */
    bool tryObtainUnicastLink(const Persona &persona, LinkType linkType,
                              const std::string &channelGid, LinkSide linkSide);

    /**
     * @brief Create a link to send from this node to the passed list of personas. This _only_ forms
     * a bidirectional link if the channel is LD_BIDI, otherwise it will be unidirectional LT_SEND
     * from this node. Further, it does _not_ inform each recipient node of the _other_ recipient
     * nodes - each recipient just knows this node is sending to them.
     *
     * @param personaList vector<Persona> of personas to send to
     * @param linkType The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
     * @param channelGid The desired channel GID
     * @param linkSide The desired side (creator, loader, both/either) of the link to be obtained
     *      on this node
     * @return bool False if a local link creation operation failed, otherwise true is returned but
     * could later fail
     */
    bool tryObtainMulticastSend(const std::vector<Persona> &personaList, LinkType linkType,
                                const std::string &channelGid, LinkSide linkSide);

    /**
     * @brief Handle a change to channel status. Currently has no behavior.
     *
     * @return True
     */
    bool handleChannelStatusUpdate(RaceHandle handle, std::string &channelGid,
                                   ChannelStatus status);

    /**
     * @brief Handle a change to link status. This watches for link statuses associated with calls
     * the LinkWizard previously made and triggers behavior for them. Primarily for createLink
     * results this generates transmission of the LinkAddress to other nodes.
     *
     * @param handle The RaceHandle of the previous link-related call
     * @param linkId Currently unused
     * @param status The new status of the link
     * @param properties The current LinkProperties of the link
     * @return bool Success of the operation. False if the handle did not correspond to a previous
     * LinkWizard call.
     */
    bool handleLinkStatusUpdate(RaceHandle handle, const LinkID &linkId, LinkStatus status,
                                const LinkProperties &properties);

    /**
     * @brief Retry previously delayed reponses to getSupportedChannels queries
     * Should only be called after all channels are out of intermediate states
     *
     */
    void retryRespondSupportedChannels();

    /**
     * @brief Retry obtain requests that were previously queued due to a lack of knowledge of
     * supported channels for the involved nodes.
     *
     * @param uuid The string uuid of the node that was updated, to more efficiently retry requests.
     */
    void tryQueuedRequests(const std::string &uuid);

private:
    // internal methods

    /**
     * @brief Attempt to construct a new unicast link with the persona of the type specified. If
     * linkType is LT_BIDI this may cause creation of two unidirectional links. This returns false
     * then either there are no shared supported channels or there was an error attempting to create
     * the link or sending the message.
     *
     * @param persona The Persona to establish a unicast link with
     * @param linkType The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
     * @param channelGid The desired channel GID
     * @param linkSide The desired side (creator, loader, both/either) of the link to be obtained
     *      on this node
     * @return bool Success of the operation.
     */
    bool obtainUnicastLink(const Persona &persona, LinkType linkType, const std::string &channelGid,
                           LinkSide linkSide);

    /**
     * @brief Create a link to send from this node to the passed list of personas. This _only_ forms
     * a bidirectional link if the channel is LD_BIDI, otherwise it will be unidirectional LT_SEND
     * from this node. Further, it does _not_ inform each recipient node of the _other_ recipient
     * nodes - each recipient just knows this node is sending to them.
     *
     * @param personaList vector<Persona> of personas to send to
     * @param linkType The type of link to obtain (e.g. LT_RECV; LT_SEND; LT_BIDI)
     * @param channelGid The desired channel GID
     * @param linkSide The desired side (creator, loader, both/either) of the link to be obtained
     *      on this node
     * @return bool False if a local link creation operation failed, otherwise true is returned but
     * could later fail
     */
    bool obtainMulticastSend(const std::vector<Persona> &personaList, LinkType linkType,
                             const std::string &channelGid, LinkSide linkSide);

    /**
     * @brief Send a LinkWizard message querying the channels supported by uuid.
     *
     * @param uuid The uuid of the persona to query.
     * @return bool Success of the operation. Failure if the uuid could not be reached.
     */
    bool querySupportedChannels(const std::string &uuid);

    /**
     * @brief Respond to a LinkWizard querySupportedChannels message with the list of channels this
     * node supports.
     *
     * @param uuid The string uuid of the persona to respond to.
     * @param ctx The opentracing context to use.
     * @return bool Success of the operation. Failure if the uuid could not be reached.
     */
    bool respondSupportedChannels(const std::string &uuid,
                                  const std::pair<std::uint64_t, std::uint64_t> &tracingIds);

    /**
     * @brief Send a LinkWizard message requesting the uuid to create a link of channelGid and share
     * the LinkAddress back.
     *
     * @param uuid The string uuid of the persona to request to
     * @param channelGid The name of the channel to request be created
     * @return bool Success of the operation. Failure indicates the message could not be sent
     */
    bool requestCreateUnicastLink(const std::string &uuidList, const std::string &channelGid);

    /**
     * @brief Send a request to this list of uuids to create a link for this channelGid and send the
     * LinkAddress back to this node. Used for obtainMulticastSend using an LD_LOADER_TO_CREATOR
     * channel.
     *
     * @param uuidList vector<string> of uuids send the request to
     * @param channelGid String the name of the channel to request
     * @param requestId String request identifier to identify the returned LinkAddress
     * @return bool False if any request fails to send, otherwise true
     */
    bool requestCreateMulticastRecvLink(const std::vector<std::string> &uuidList,
                                        const std::string &channelGid,
                                        const std::string &requestId);

    /**
     * @brief Handle a createLink request message. Create the link if we can and then queue an
     * action to send the LinkAddress back to uuid when the link is created.
     *
     * @param uuid The string uuid of the persona requesting the create
     * @param channelGid The name of the channel to create
     * @return bool Success of the operation. Failure indicates the channel is not supported or
     * could not be created
     */
    bool handleCreateUnicastLinkRequest(const std::string &uuid, const std::string &channelGid);

    /**
     * @brief Respond with the LinkAddress from a createLink called based on a
     * requestCreateMulticastRecvLink message.
     *
     * @param uuid String uuid to send the LinkAddress to
     * @param requestId String request identifier to disambiguate this response on the recipient
     * @param channelGid String name of the channel created
     * @param linkAddress String the LinkAddress of the link created in response to the original
     * request
     * @return bool True if the message succeeds being sent
     */
    bool respondRequestedCreateMulticastRecv(const std::string &uuid, const std::string &requestId,
                                             const std::string &channelGid,
                                             const std::string &linkAddress);
    /**
     * @brief Handle responses to a requestCreateMulticastRecv by caching the LinkAddress and, if
     * all LinkAddresses have been received, calling loadLinkAddresses on the cached addresses.
     *
     * @param uuid String uuid of the node responding
     * @param requestId String request identifier to match to the original
     * requestCreateMulticastRecv
     * @param channelGid String name of the channel
     * @param linkAddress String LinkAddress of the link to load
     * @return bool true if all LinkAddresses have arrived and the link loaded, otherwise false
     */
    bool handleCreateMulticastRecvResponse(const std::string &uuid, const std::string &requestId,
                                           const std::string &channelGid,
                                           const std::string &linkAddress);
    /**
     * @brief Handle a request to create receive link for this channel and respond with its
     * LinkAddress. Differs from handleCreateUnicastLinkRequest because a requestId exists and is
     * kept for responding with the LinkAddress.
     *
     * @param uuid String uuid of the node requesting
     * @param channelGid String name of the channel
     * @param requestId String request identifier
     * @return bool False if createLink fails or the channel is unsupported, true otherwise
     */
    bool handleCreateMulticastRecvLinkRequest(const std::string &uuid,
                                              const std::string &channelGid,
                                              const std::string &requestId);
    /**
     * @brief Send a LinkWizard message requesting the uuid to load a LinkAddress.
     *
     * @param uuid The string uuid of the persona to request
     * @param channelGid The name of the channel to call loadLinkAddress on
     * @param linkAddress The LinkAddress to load
     * @param uuidList The list of persona uuid's that should be associated with this loaded link
     * (for multicast groups)
     * @return bool Success of the operation. Failure indicates the message could not be sent.
     */
    bool requestLoadLinkAddress(const std::string &uuid, const std::string &channelGid,
                                const std::string &linkAddress,
                                const std::vector<std::string> &personas);
    /**
     * @brief Handle a loadLinkAddress request. Call loadLinkAddress with the passed channelGid and
     * linkAddress and the uuidList with our own persona removed from it (if it was present).
     *
     * @param uuid The string uuid of the persona requesting the load
     * @param channelGid The name of the channel to load
     * @param linkAddress The LinkAddress to load
     * @param uuidList The list of persona uuid's that are involved in this link. This may contain
     * our own uuid, which should be removed before calling loadLinkAddress.
     * @return bool Success of the operation. Failure indicates the channel is not supported or
     * failed to load.
     */
    bool handleLoadLinkAddressRequest(const std::string &uuid, const std::string &channelGid,
                                      const std::string &linkAddress,
                                      const std::vector<std::string> &uuidList);

    /**
     * @brief Helper function to add a new listing relating a requestId to a map of uuid:address
     *
     * @param requestId String the key value for the multiAddressLoads
     * @param uuidList vector<string> list of uuids we care about
     */
    void addPendingMultiAddressLoads(std::string requestId, std::vector<std::string> uuidList);

    /**
     * @brief Generate a new unique request identifier. Based on a local counter and an assumption
     * that UUIDs are unique to each node.
     *
     * @return string requestId of uuid-#
     */
    std::string generateRequestId();

    /**
     * @brief Helper that sets opentracing SpanId and TraceId
     *
     * @param msg ExtClrMsg to set span and trace IDs on
     * @param spanName String to set as the name
     * @param ctx Optional opentracing context for the span to inherit from
     */
    void setOpenTracing(ExtClrMsg &msg, const std::string &spanName,
                        const opentracing::SpanContext *ctx = nullptr);

    /**
     * @brief Helper that checks if the supported channels for every uuid in a list are known. This
     * gates obtain___ calls.
     *
     * @param uuidList vector<string> of uuids to check for supported channels for
     * @return bool True if all uuids in uuidList have supported channels known.
     */
    bool channelsKnownForAllUuids(const std::vector<std::string> &uuidList);

    /**
     * @brief Helper function that verifies the desired link type, channel, and link side are
     * valid and supported by all the specified uuids.
     *
     * @param uuidList List of UUIDs for which to verify channel support
     * @param linkType Desired link type
     * @param unicast Whether link is unicast or multicast
     * @param anyClients Whether there are any client nodes involved
     * @param channelGid Desired channel GID
     * @param linkSide Desired link side (creator, loader, or both/either)
     *
     * @return Tuple of boolean channel valid, channel properties, and resulting link side (in case
     *      the channel supports both and we desire either, but the other side does not support
     *      both)
     */
    std::tuple<bool, ChannelProperties, LinkSide> verifyChannelIsSupported(
        const std::vector<std::string> &uuidList, LinkType linkType, bool unicast, bool anyClients,
        const std::string &channelGid, LinkSide linkSide);

    /**
     * @brief Helper function that selects a channel given a list of uuids that need to support it,
     * a desired LinkType, whether the link will be unicast or multicast, and whether any of the
     * nodes involved are clients (meaning only CT_INDIRECT channels).
     *
     * @param uuidList vector<string> list of uuids to select for
     * @param linkType LinkType of the link to eventually create
     * @param unicast bool whether this link is unicast or multicast
     * @param anyClients bool whether there are any client nodes involved
     * @return tuple<string, ChannelProperties, LinkSide> of the selected channelGid, properties,
     * and creator or loader side of the link (in case the channel supports both but the other side
     * does not). If the channelGid is empty, then selection failed.
     */
    std::tuple<std::string, ChannelProperties, LinkSide> selectChannel(
        const std::vector<std::string> &uuidList, LinkType linkType, bool unicast, bool anyClients);

    /**
     * @brief Helper function to convert a list of Persona objects into a stringified concatenation
     * of their uuids and a vector<string> of their uuids
     *
     * @param personaList vector<Persona> to transform
     * @return pair<string, vector<string> the space-separated list and vector representation of the
     * uuids of personaList members
     */
    std::pair<std::string, std::vector<std::string>> personaListToUuidList(
        const std::vector<Persona> &personaList);

    // External classes to call to
    PluginNMTwoSix *plugin;
    RaceCrypto encryptor;

    // Internal variables
    const std::string raceUuid;
    const PersonaType personaType;
    int currentRequestId;
    bool readyToRespond;

    struct QueuedObtainUnicast {
        Persona persona;
        LinkType linkType;
        std::string channelGid;
        LinkSide linkSide;

        QueuedObtainUnicast(Persona persona, LinkType linkType, std::string channelGid,
                            LinkSide linkSide) :
            persona(std::move(persona)),
            linkType(linkType),
            channelGid(std::move(channelGid)),
            linkSide(linkSide) {}
    };

    struct QueuedObtainMulticast {
        std::vector<Persona> personaList;
        LinkType linkType;
        std::string channelGid;
        LinkSide linkSide;

        QueuedObtainMulticast(std::vector<Persona> personaList, LinkType linkType,
                              std::string channelGid, LinkSide linkSide) :
            personaList(std::move(personaList)),
            linkType(linkType),
            channelGid(std::move(channelGid)),
            linkSide(linkSide) {}
    };

    // Internal mappings
    using ChannelToLinkSideMap = std::unordered_map<std::string, LinkSide>;
    std::unordered_map<std::string, ChannelToLinkSideMap> uuidToSupportedChannelsMap;
    std::unordered_map<std::string, std::vector<LinkType>> requestMap;
    std::vector<std::pair<std::string, std::pair<std::uint64_t, std::uint64_t>>>
        respondSupportedChannelsQueue;
    std::unordered_map<std::string, std::vector<QueuedObtainUnicast>> obtainUnicastQueue;
    std::vector<QueuedObtainMulticast> obtainMulticastSendQueue;
    std::vector<std::tuple<std::string, std::string, std::uint64_t, std::uint64_t>>
        sendingMessageQueue;
    std::unordered_set<std::string> outstandingQueries;
    std::unordered_map<RaceHandle, std::string> pendingUnicastCreate;
    std::unordered_map<RaceHandle, std::vector<std::string>> pendingMulticastSendCreate;
    std::unordered_map<int, std::pair<std::string, std::string>>
        pendingRequestedMulticastRecvCreate;
    std::unordered_map<RaceHandle, std::vector<std::string>> pendingLoad;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
        pendingMultiAddressLoads;

    /**
     * @brief The opentracing tracer used to do opentracing logging
     *
     */
    std::shared_ptr<opentracing::Tracer> tracer;
};
#endif
