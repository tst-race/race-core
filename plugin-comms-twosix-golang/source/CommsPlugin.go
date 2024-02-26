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

// CommsPluginTwoSix Interface. Is a Golang  implementation of the RACE T2 Plugin. Will
// perform obfuscated communication for the RACE system.

package main

import "C"

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	commsShims "shims"
	"strconv"
	"strings"
	"sync"
	"time"
	"unsafe"
)

const (
	CONN_TYPE = "tcp"
)

// A CommsConn represents a logical connection connecting two RACE nodes
type CommsConn interface {
	// Returns the link ID of the connection
	GetLinkId() string
	// Returns the link type of the connection
	GetLinkType() commsShims.LinkType
	// Adds a connection ID to the connection, returning the new number of IDs
	AddConnectionId(connectionId string) int
	// Removes a connection ID from the connection, returning the new number of IDs
	RemoveConnectionId(connectionId string) int
	// Gets the list of connection IDs associated with the connection
	GetConnectionIds() []string
	// Closes the connection
	Close() error
	// Writes the given raw message payload to the connection
	Write(msg []byte) error
	// Starts receiving messages over the connection. This method should block
	// until the connection has been closed. It will be invoked in a goroutine.
	Receive(plugin *overwrittenMethodsOnCommsPluginTwoSix)
}

// Attributes common to unicast and multicast connection types
type commsConnCommon struct {
	connectionIdsAsMap map[string]bool
	connectionIdsMutex sync.RWMutex
	LinkId             string
	LinkType           commsShims.LinkType
}

// Returns the link ID of the given connection
func (conn *commsConnCommon) GetLinkId() string {
	return conn.LinkId
}

// Returns the link type of the given connection
func (conn *commsConnCommon) GetLinkType() commsShims.LinkType {
	return conn.LinkType
}

// Adds a connection ID to the connection, returning the new number of IDs
func (conn *commsConnCommon) AddConnectionId(connectionId string) int {
	conn.connectionIdsMutex.Lock()
	defer conn.connectionIdsMutex.Unlock()
	if connectionId != "" {
		conn.connectionIdsAsMap[connectionId] = true
	} else {
		logWarning("commsConnCommon::AddConnectionId: invalid connection ID is empty string.")
	}
	return len(conn.connectionIdsAsMap)
}

// Removes a connection ID from the connection, returning the new number of IDs
func (conn *commsConnCommon) RemoveConnectionId(connectionId string) int {
	conn.connectionIdsMutex.Lock()
	defer conn.connectionIdsMutex.Unlock()
	delete(conn.connectionIdsAsMap, connectionId)
	return len(conn.connectionIdsAsMap)
}

// Gets the list of connection IDs associated with the connection
func (conn *commsConnCommon) GetConnectionIds() []string {
	conn.connectionIdsMutex.RLock()
	defer conn.connectionIdsMutex.RUnlock()
	var keys []string
	for key := range conn.connectionIdsAsMap {
		keys = append(keys, key)
	}
	return keys
}

// Unicast/direct connection type
type commsConnUnicast struct {
	commsConnCommon
	Host string
	Port int
	Sock net.Listener
}

// Unicast/direct connection parameters
type unicastProfile struct {
	Hostname string `json:"hostname"`
	Port     int    `json:"port"`
}

// Creates a new unicast connection instance
func newUnicastConn(newConnectionId string, linkType commsShims.LinkType, linkId string, linkProfile string) (CommsConn, error) {
	var profile unicastProfile
	err := json.Unmarshal([]byte(linkProfile), &profile)
	if err != nil {
		logError("failed to parse link profile json: ", err.Error())
		return nil, err
	}
	if newConnectionId == "" {
		logError("newUnicastConn: invalid connection ID is empty string")
		return nil, err
	}

	// Create the connection object
	connection := commsConnUnicast{
		commsConnCommon: commsConnCommon{
			connectionIdsAsMap: map[string]bool{newConnectionId: true},
			LinkId:             linkId,
			LinkType:           linkType,
		},
		Host: profile.Hostname,
		Port: profile.Port,
	}

	logDebug("OpenConnection:opened connection on host \"", connection.Host, "\" and port \"", connection.Port, "\"")
	return &connection, nil
}

// Close the socket associated with the given Connection
// (This will cause the active goroutine that
// is listening on this socket to end.)
func (connection *commsConnUnicast) Close() error {
	if connection.Sock != nil {
		return connection.Sock.Close()
	}
	return nil
}

// Open a connection to the destination host and write the given payload message
func (connection *commsConnUnicast) Write(msg []byte) error {
	logDebug("Sending Message to ", connection.Host, ":", connection.Port)

	// connect to this Socket
	conn, dialErr := net.Dial(CONN_TYPE, fmt.Sprintf("%v:%v", connection.Host, connection.Port))
	if dialErr != nil {
		logError("Error Connecting to Send Socket: ", dialErr.Error())
		return dialErr
	}
	defer conn.Close()

	// Send Message to Socket
	_, writeErr := conn.Write(msg)
	return writeErr
}

// Open a server socket and accept incoming messages. All received messages will be forwarded
// to the given plugin. This must be executed within a goroutine.
func (connection *commsConnUnicast) Receive(plugin *overwrittenMethodsOnCommsPluginTwoSix) {
	logDebug("connectionMonitor:    host: ", connection.Host)
	logDebug("connectionMonitor:    port: ", connection.Port)

	// Create a listening socket
	l, err := net.Listen(CONN_TYPE, fmt.Sprintf("%v:%v", connection.Host, connection.Port))
	if err != nil {
		logError("connectionMonitor: Error Connecting to Listen Socket: ", err.Error())
		os.Exit(1)
	}
	defer l.Close()

	// Add the socket to the connection object (so that it can be accessed from elsewhere
	connection.Sock = l
	buffer := make([]byte, 1024)

	logDebug("connectionMonitor: Listening on ", connection.Host, ":", connection.Port)

	for true {
		conn, err := l.Accept()
		if err != nil {
			if strings.HasSuffix(err.Error(), ": use of closed network connection") {
				// If the socket is closed, it is likely because the connection was closed from outside the goroutine, so we don't have a fit. Break the accept loop.
				logDebug("connectionMonitor: Socket closed")
				break
			}
			logError("connectionMonitor: Error accepting: ", err.Error())
			os.Exit(1)
		}
		// Read data until the socket is closed
		data := make([]byte, 0)

		n, err := conn.Read(buffer)
		for err == nil && n > 0 {
			data = append(data, buffer[:n]...)
			n, err = conn.Read(buffer)
		}

		if err != nil && err != io.EOF {
			logError("connectionMonitor: Problem reading data from socket: ", err)
		}
		logDebug("connectionMonitor: Read ", len(data), " byte message")

		rawData := commsShims.NewByteVector()
		for _, b := range data {
			rawData.Add(b)
		}

		receivedEncPkg := commsShims.NewEncPkg(rawData)
		plugin.raceSdkReceiveEncPkgWrapper(receivedEncPkg, connection.GetConnectionIds())
		commsShims.DeleteByteVector(rawData)
		commsShims.DeleteEncPkg(receivedEncPkg)
	}
}

// Multicast/whiteboard connection type
type commsConnMulticast struct {
	commsConnCommon
	baseUrl        string
	hashtag        string
	checkFrequency time.Duration
	client         http.Client
	stop           chan bool
}

// Multicast/whiteboard connection parameters
type multicastProfile struct {
	Hostname             string `json:"hostname"`
	Port                 uint   `json:"port"`
	Hashtag              string `json:"hashtag"`
	CheckFrequencyMillis uint   `json:"checkFrequency"`
}

// Creates a new multicast connection instance
func newMulticastConn(newConnectionId string, linkType commsShims.LinkType, linkId string, linkProfile string) (CommsConn, error) {
	var profile multicastProfile
	err := json.Unmarshal([]byte(linkProfile), &profile)
	if err != nil {
		logError("failed to parse link profile json: ", err.Error())
		return nil, err
	}
	if newConnectionId == "" {
		logError("newMulticastConn: invalid connection ID is empty string")
		return nil, err
	}

	// Create the connection object
	connection := commsConnMulticast{
		commsConnCommon: commsConnCommon{
			connectionIdsAsMap: map[string]bool{newConnectionId: true},
			LinkId:             linkId,
			LinkType:           linkType,
		},
		baseUrl:        fmt.Sprintf("http://%v:%v", profile.Hostname, profile.Port),
		hashtag:        profile.Hashtag,
		checkFrequency: time.Duration(profile.CheckFrequencyMillis) * time.Millisecond,
		client: http.Client{
			Timeout: time.Second * 10,
		},
		stop: make(chan bool, 1), // Buffered channel of size 1 (should only get closed once)
	}

	logDebug("OpenConnection:opened connection to ", connection.baseUrl, " with hashtag ", connection.hashtag)
	return &connection, nil
}

// Sends a stop value through the channel to the goroutine
func (connection *commsConnMulticast) Close() error {
	select {
	case connection.stop <- true:
	default:
		logWarning("Close: unable to send stop message for ", connection.baseUrl, " with hashtag ", connection.hashtag)
	}
	return nil
}

// Executes a POST HTTP request to the whiteboard service
func (connection *commsConnMulticast) Write(msg []byte) error {
	payload := make(map[string]string)
	payload["data"] = base64.StdEncoding.EncodeToString(msg)

	jsonPayload, jsonErr := json.Marshal(payload)
	if jsonErr != nil {
		logError("Failed to create JSON payload for new message: ", jsonErr.Error())
		return jsonErr
	}

	url := fmt.Sprintf("%v/post/%v", connection.baseUrl, connection.hashtag)
	resp, reqErr := connection.client.Post(url, "application/json", bytes.NewBuffer(jsonPayload))
	if reqErr != nil {
		logError("Failed to post message: ", reqErr.Error())
		return reqErr
	}

	logDebug("Posted message, status = ", resp.Status)
	return nil
}

// Executes a GET HTTP request to the specified URL
func (connection *commsConnMulticast) get(url string, target interface{}) error {
	resp, err := connection.client.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	return json.NewDecoder(resp.Body).Decode(target)
}

// Payload for whiteboard service latest-index API
type latestIndex struct {
	Latest int `json:"latest"`
}

// Payload for whiteboard service messages API
type messages struct {
	Data   []string `json:"data"`
	Length int      `json:"length"`
}

// Periodically polls the whiteboard service for new messages. All received messages will be
// forwarded to the given plugin. This must be executed within a goroutine.
func (connection *commsConnMulticast) Receive(plugin *overwrittenMethodsOnCommsPluginTwoSix) {
	latest := 0

	// Get the latest message index from the whiteboard service
	{
		url := fmt.Sprintf("%v/latest/%v", connection.baseUrl, connection.hashtag)
		payload := latestIndex{}
		err := connection.get(url, &payload)
		if err != nil {
			logError("Unable to get last post index: ", err.Error())
		} else {
			latest = payload.Latest
		}
	}

	for {
		// Calculate the time of next run before we poll so we run at a constant rate
		delay := time.After(connection.checkFrequency)

		url := fmt.Sprintf("%v/get/%v/%v/-1", connection.baseUrl, connection.hashtag, latest)
		payload := messages{}
		err := connection.get(url, &payload)
		if err != nil {
			logError("Unable to get new posts: ", err.Error())
		} else {
			expectedNum := payload.Length - latest
			actualNum := len(payload.Data)
			if actualNum < expectedNum {
				lostNum := expectedNum - actualNum
				logError(fmt.Sprintf("Expected %v posts, but only got %v, %v posts may have been lost.",
					expectedNum, actualNum, lostNum))
			}

			latest = payload.Length

			connectionIds := connection.GetConnectionIds()

			for _, message := range payload.Data {
				dataByteSlice, err := base64.StdEncoding.DecodeString(message)
				if err != nil {
					logError("Unable to base64 decode message: ", err.Error())
				} else {
					dataByteVector := commsShims.NewByteVector()
					for _, b := range dataByteSlice {
						dataByteVector.Add(b)
					}
					logDebug("Receive: received multicast message on connection with IDs: ", strings.Join(connectionIds, ", "))
					encPkg := commsShims.NewEncPkg(dataByteVector)
					plugin.raceSdkReceiveEncPkgWrapper(encPkg, connectionIds)
				}
			}
		}

		select {
		case <-delay:
			// Do nothing (re-run the loop)
		case <-connection.stop:
			// We were told to stop
			return
		}
	}
}

// Forces interface to be a superset of the abstract base class
// Go type to define abstract methods.
type overwrittenMethodsOnCommsPluginTwoSix struct {
	sdk                    commsShims.IRaceSdkComms
	connections            map[string]CommsConn
	connectionsMutex       sync.RWMutex
	linkProfiles           map[string]string
	linkProperties         map[string]commsShims.LinkProperties
	channelStatuses        map[string]commsShims.ChannelStatus
	recvChannel            chan int
	nextAvailablePort      int
	whiteboardHostname     string
	whiteboardPort         int
	nextAvailableHashTag   int
	hostname               string
	requestStartPortHandle uint64
	requestHostnameHandle  uint64
}

// Wrapper for debug level logging using the RACE Logging API call
func logDebug(msg ...interface{}) {
	commsShims.RaceLogLogDebug("CommsPluginTwoSixGolang", fmt.Sprint(msg...), "")
}

// Wrapper for info level logging using the RACE Logging API call
func logInfo(msg ...interface{}) {
	commsShims.RaceLogLogInfo("CommsPluginTwoSixGolang", fmt.Sprint(msg...), "")
}

// Wrapper for warn level logging using the RACE Logging API call
func logWarning(msg ...interface{}) {
	commsShims.RaceLogLogWarning("CommsPluginTwoSixGolang", fmt.Sprint(msg...), "")
}

// Wrapper for error level logging using the RACE Logging API call
func logError(msg ...interface{}) {
	commsShims.RaceLogLogError("CommsPluginTwoSixGolang", fmt.Sprint(msg...), "")
}

// LinkPropSetJson represents a list of properties associated with the link. These include
// information useful for the network manager/core to choose which links to use for different types of
// communication
type LinkPropSetJson struct {
	Bandwidth_bps int     `json:"bandwidth_bps"`
	Latency_ms    int     `json:"latency_ms"`
	Loss          float32 `json:"loss"`
}

// Creates and returns a new LinkPropSet
func NewLinkPropertySet(json LinkPropSetJson) commsShims.LinkPropertySet {
	propSet := commsShims.NewLinkPropertySet()
	propSet.SetBandwidth_bps(json.Bandwidth_bps)
	propSet.SetLatency_ms(json.Latency_ms)
	propSet.SetLoss(json.Loss)
	return propSet
}

// LinkPropPairJson holds the send and receive properites of a connection. This
// includes a LinkPropSetJson for the send and receive side of the connection.
type LinkPropPairJson struct {
	Send    LinkPropSetJson `json:"send"`
	Receive LinkPropSetJson `json:"receive"`
}

// Creates and returns a new LinkPropPair
func NewLinkPropertyPair(json LinkPropPairJson) commsShims.LinkPropertyPair {
	propPair := commsShims.NewLinkPropertyPair()
	propPair.SetSend(NewLinkPropertySet(json.Send))
	propPair.SetReceive(NewLinkPropertySet(json.Receive))
	return propPair
}

// LinkPropJson represents the complete properties for a given link. This includes
// details about the link, properties (best/worst/expected cases), and what
// type of link the link is
type LinkPropJson struct {
	Linktype        string           `json:"type"`
	Reliable        bool             `json:"reliable"`
	Duration_s      int              `json:"duration_s"`
	Period_s        int              `json:"period_s"`
	Mtu             int              `json:"mtu"`
	Worst           LinkPropPairJson `json:"worst"`
	Best            LinkPropPairJson `json:"best"`
	Expected        LinkPropPairJson `json:"expected"`
	Unicast         bool             `json:"unicast"`
	Multicast       bool             `json:"multicast"`
	Supported_hints []string         `json:"supported_hints"`
}

// Unmarshal the data object into a LinkPropJson
func (t *LinkPropJson) UnmarshalJSON(data []byte) error {
	type alias LinkPropJson
	tmpSet := LinkPropSetJson{
		Bandwidth_bps: -1,
		Latency_ms:    -1,
		Loss:          -1.0,
	}
	tmpPair := LinkPropPairJson{
		Send:    tmpSet,
		Receive: tmpSet,
	}
	tmp := &alias{
		Duration_s: -1,
		Period_s:   -1,
		Mtu:        -1,
		Worst:      tmpPair,
		Best:       tmpPair,
		Expected:   tmpPair,
	}
	if err := json.Unmarshal(data, &tmp); err != nil {
		return err
	}

	*t = LinkPropJson(*tmp)
	return nil
}

// LinkProfileJson represents a LinkProfile which defines what a link is, how it
// connects, who it connects to, and which nodes an utilize the link
type LinkProfileJson struct {
	ConnectedTo []string     `json:"connectedTo"`
	UtilizedBy  []string     `json:"utilizedBy"`
	Profile     string       `json:"profile"`
	Properties  LinkPropJson `json:"properties"`
}

// Set the Sdk object and perform minimum work to
// be able to respond to incoming calls.
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) Init(pluginConfig commsShims.PluginConfig) commsShims.PluginResponse {
	logInfo("Init called")
	defer logInfo("Init returned")

	logDebug("etcDirectory: ", pluginConfig.GetEtcDirectory())
	logDebug("auxDataDirectory: ", pluginConfig.GetAuxDataDirectory())
	logDebug("loggingDirectory: ", pluginConfig.GetLoggingDirectory())
	logDebug("tmpDirectory: ", pluginConfig.GetTmpDirectory())
	logDebug("pluginDirectory: ", pluginConfig.GetPluginDirectory())

	plugin.channelStatuses = map[string]commsShims.ChannelStatus{
		DIRECT_CHANNEL_GID:   commsShims.CHANNEL_UNAVAILABLE,
		INDIRECT_CHANNEL_GID: commsShims.CHANNEL_UNAVAILABLE,
	}

	plugin.nextAvailablePort = 10000
	plugin.whiteboardHostname = "twosix-whiteboard"
	plugin.whiteboardPort = 5000
	plugin.nextAvailableHashTag = 0
	plugin.hostname = "no-hostname-provided-by-user"

	plugin.connections = make(map[string]CommsConn)
	plugin.linkProfiles = make(map[string]string)
	plugin.linkProperties = make(map[string]commsShims.LinkProperties)

	bytesToWrite := commsShims.NewByteVector()
	for _, b := range []byte("Comms Golang Plugin Initialized\n") {
		bytesToWrite.Add(b)
	}
	responseStatus := plugin.sdk.WriteFile("initialized.txt", bytesToWrite).GetStatus()
	if responseStatus != commsShims.SDK_OK {
		logError("Failed to write initialized.txt")
	}
	bytesRead := plugin.sdk.ReadFile("initialized.txt")
	bytes := []byte{}
	if bytesRead.Size() >= 2<<32 {
		logError("File too large, only reading first 2^32 bytes")
	}
	for idx := 0; idx < int(bytesRead.Size()); idx++ {
		bytes = append(bytes, bytesRead.Get(idx))
	}
	stringRead := string(bytes)
	logDebug("Read Initialization File: ", stringRead)

	return commsShims.PLUGIN_OK
}

// Shutdown the plugin. Close open connections, remove state, etc.
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) Shutdown() commsShims.PluginResponse {
	logInfo("Shutdown: called")
	handle := commsShims.GetNULL_RACE_HANDLE()
	for connectionId, _ := range plugin.connections {
		plugin.CloseConnection(handle, connectionId)
	}
	logInfo("Shutdown: returned")
	return commsShims.PLUGIN_OK
}

// Get link properties for the specified link
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) GetLinkProperties(linkType commsShims.LinkType, linkId string) commsShims.LinkProperties {
	logInfo("GetLinkProperties called")
	if props, ok := plugin.linkProperties[linkId]; ok {
		return props
	}
	return commsShims.NewLinkProperties()
}

// Get connection properties for the specified connection
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) GetConnectionProperties(linkType commsShims.LinkType, connectionId string) commsShims.LinkProperties {
	logInfo("GetConnectionProperties called")
	if conn, conn_exists := plugin.connections[connectionId]; conn_exists {
		if props, link_exists := plugin.linkProperties[conn.GetLinkId()]; link_exists {
			return props
		}
	}
	return commsShims.NewLinkProperties()
}

// Send an encrypted package
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) SendPackage(handle uint64, connectionId string, encPkg commsShims.EncPkg, timeoutTimestamp float64, batchId uint64) commsShims.PluginResponse {
	defer commsShims.DeleteEncPkg(encPkg)

	logInfo("SendPackage called")
	defer logInfo("SendPackage returned")

	// get the raw bytes out of the Encrypted Package
	msg_vec := encPkg.GetRawData()
	defer commsShims.DeleteByteVector(msg_vec)
	msg := make([]byte, 0, msg_vec.Size())
	msg_size := int(msg_vec.Size())
	for i := 0; i < msg_size; i++ {
		msg = append(msg, msg_vec.Get(i))
	}

	// get the connection associated with the specified connection ID
	plugin.connectionsMutex.RLock()
	connection, ok := plugin.connections[connectionId]
	plugin.connectionsMutex.RUnlock()
	if !ok {
		logError("failed to find connection with ID = ", connectionId)
		plugin.sdk.OnPackageStatusChanged(handle, commsShims.PACKAGE_FAILED_GENERIC, commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	if err := connection.Write(msg); err != nil {
		plugin.sdk.OnPackageStatusChanged(handle, commsShims.PACKAGE_FAILED_GENERIC, commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	plugin.sdk.OnPackageStatusChanged(handle, commsShims.PACKAGE_SENT, commsShims.GetRACE_BLOCKING())
	return commsShims.PLUGIN_OK
}

// Open a connection with a given type on the specified link. Additional configuration
// info can be provided via the linkHints param.
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) OpenConnection(handle uint64, linkType commsShims.LinkType, linkId string, link_hints string, send_timeout int) commsShims.PluginResponse {
	logInfo("OpenConnection: called")
	logDebug("OpenConnection:    type = ", linkType)
	logDebug("OpenConnection:    Link ID = ", linkId)
	logDebug("OpenConnection:    link_hints = ", link_hints)
	logDebug("OpenConnection:    send_timeout = ", send_timeout)
	defer logInfo("OpenConnection: returned")

	if _, ok := plugin.linkProperties[linkId]; !ok {
		logError("OpenConnection:failed to find link with ID = ", linkId)
		return commsShims.PLUGIN_ERROR
	}

	newConnectionId := plugin.sdk.GenerateConnectionId(linkId)
	logDebug("OpenConnection: opening new connection with ID: ", newConnectionId)
	linkProperties := plugin.linkProperties[linkId]

	// Check if there is already an open connection that can be reused.
	plugin.connectionsMutex.Lock()
	for _, connection := range plugin.connections {
		if connection.GetLinkId() == linkId && connection.GetLinkType() == linkType {
			connection.AddConnectionId(newConnectionId)
			plugin.connections[newConnectionId] = connection
			plugin.connectionsMutex.Unlock()
			plugin.sdk.OnConnectionStatusChanged(handle, newConnectionId, commsShims.CONNECTION_OPEN, linkProperties, commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_OK
		}
	}
	plugin.connectionsMutex.Unlock()

	// Get the Link Profile with the specified ID
	linkProfile, ok := plugin.linkProfiles[linkId]
	if !ok {
		logError("OpenConnection:failed to find link profile for link with ID = ", linkId)
		plugin.sdk.OnConnectionStatusChanged(handle, newConnectionId, commsShims.CONNECTION_CLOSED, linkProperties, commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	logDebug("OpenConnection:opening connection for link profile: ", linkProfile)

	var connection CommsConn
	var err error
	if linkProperties.GetTransmissionType() == commsShims.TT_MULTICAST {
		connection, err = newMulticastConn(newConnectionId, linkType, linkId, linkProfile)
	} else {
		connection, err = newUnicastConn(newConnectionId, linkType, linkId, linkProfile)
	}

	if err != nil {
		logError("OpenConnection: failed to create connection: ", err)
		plugin.sdk.OnConnectionStatusChanged(handle, newConnectionId, commsShims.CONNECTION_CLOSED, linkProperties, commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	// Add the connection to the Plugin's list of all active connections
	plugin.connectionsMutex.Lock()
	plugin.connections[newConnectionId] = connection
	plugin.connectionsMutex.Unlock()

	// Start a listener (in a new goroutine) if the Link Type allows receipt of messages
	if linkType == commsShims.LT_RECV || linkType == commsShims.LT_BIDI {
		logDebug("OpenConnection:Starting Connection Monitor with connection ID(s): ", strings.Join(connection.GetConnectionIds(), ", "))
		go plugin.connectionMonitor(connection)
	}

	// Update the SDK about the connection being open
	plugin.sdk.OnConnectionStatusChanged(handle, newConnectionId, commsShims.CONNECTION_OPEN, linkProperties, commsShims.GetRACE_BLOCKING())

	// Return success
	return commsShims.PLUGIN_OK

}

// Close a connection with a given ID.
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) CloseConnection(handle uint64, connectionId string) commsShims.PluginResponse {
	logInfo("CloseConnection: called")
	defer logInfo("CloseConnection: returned")

	plugin.connectionsMutex.Lock()
	defer plugin.connectionsMutex.Unlock()
	if connection, ok := plugin.connections[connectionId]; ok {
		logDebug("CloseConnection: closing connection with ID ", connectionId)
		if connection.RemoveConnectionId(connectionId) == 0 {
			logDebug("CloseConnection: last connection ID has closed, shutting down connection")
			if err := connection.Close(); err != nil {
				logError("CloseConnection: error occurred closing connection ", connectionId, ": ", err.Error())
			}
		}
		delete(plugin.connections, connectionId)

		// Update the SDK that the connection has been closed
		plugin.sdk.OnConnectionStatusChanged(handle, connectionId, commsShims.CONNECTION_CLOSED, plugin.linkProperties[connection.GetLinkId()], commsShims.GetRACE_BLOCKING())
	} else {
		logError("CloseConnection:unable to find connection with ID = ", connectionId)
		return commsShims.PLUGIN_ERROR
	}

	// Return success to the SDK
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) DestroyLink(handle uint64, linkId string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("DestroyLink: (handle: %v, link ID: %v): ", handle, linkId)
	logDebug(logPrefix, "called")
	if _, ok := plugin.linkProperties[linkId]; !ok {
		logDebug(logPrefix, "unknown link ID")
		return commsShims.PLUGIN_ERROR
	}

	plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())

	// Close all the connections for the given link.
	for connectionId, connection := range plugin.connections {
		if connection.GetLinkId() == linkId {
			// Makes call to OnConnectionStatusChanged.
			plugin.CloseConnection(handle, connectionId)
		}
	}

	delete(plugin.linkProfiles, linkId)
	delete(plugin.linkProperties, linkId)

	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) CreateLink(handle uint64, channelGid string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("CreateLink: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called")

	if status, ok := plugin.channelStatuses[channelGid]; !ok || status != commsShims.CHANNEL_AVAILABLE {
		logError(logPrefix, "channel not available")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	linkId := plugin.sdk.GenerateLinkId(channelGid)
	if linkId == "" {
		logError("CreateLink: SDK failed to generate link ID. Is th channel GID valid? ", channelGid)
		return commsShims.PLUGIN_ERROR
	}

	linkProps, err := getDefaultLinkPropertiesForChannel(plugin.sdk, channelGid)
	if err != nil {
		logError(logPrefix, "failed to get default channel properties: ", err)
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	if channelGid == DIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Creating TwoSix direct link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_RECV)

		linkProfile := unicastProfile{
			Hostname: plugin.hostname,
			Port:     plugin.nextAvailablePort,
		}
		plugin.nextAvailablePort += 1
		linkProfileJson, jsonErr := json.Marshal(linkProfile)
		if jsonErr != nil {
			logError(logPrefix, "failed to convert link profile to json: ", jsonErr.Error())
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}
		linkProps.SetLinkAddress(string(linkProfileJson))

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = string(linkProfileJson)

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_CREATED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "created direct link with link address: ", string(linkProfileJson))
	} else if channelGid == INDIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Creating TwoSix indirect link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_BIDI)

		linkProfile := multicastProfile{
			Hostname:             plugin.whiteboardHostname,
			Port:                 uint(plugin.whiteboardPort),
			Hashtag:              fmt.Sprintf("golang_%v_%v", plugin.sdk.GetActivePersona(), plugin.nextAvailableHashTag),
			CheckFrequencyMillis: 1000,
		}
		plugin.nextAvailableHashTag += 1
		linkProfileJson, jsonErr := json.Marshal(linkProfile)
		if jsonErr != nil {
			logError(logPrefix, "failed to convert link profile to json: ", jsonErr.Error())
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}
		linkProps.SetLinkAddress(string(linkProfileJson))

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = string(linkProfileJson)

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_CREATED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "created indirect link with link address: ", string(linkProfileJson))
	} else {
		logError(logPrefix, "invalid channel GID")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) CreateLinkFromAddress(handle uint64, channelGid string, linkAddress string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("CreateLinkFromAddress: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called")

	if status, ok := plugin.channelStatuses[channelGid]; !ok || status != commsShims.CHANNEL_AVAILABLE {
		logError(logPrefix, "channel not available")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	linkId := plugin.sdk.GenerateLinkId(channelGid)
	if linkId == "" {
		logError("CreateLinkFromAddress: SDK failed to generate link ID. Is th channel GID valid? ", channelGid)
		return commsShims.PLUGIN_ERROR
	}

	linkProps, err := getDefaultLinkPropertiesForChannel(plugin.sdk, channelGid)
	if err != nil {
		logError(logPrefix, "failed to get default channel properties: ", err)
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	linkProps.SetLinkAddress(string(linkAddress))
	if channelGid == DIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Creating TwoSix direct link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_RECV)

		var profile unicastProfile
		err := json.Unmarshal([]byte(linkAddress), &profile)
		if err != nil {
			logError(logPrefix, "failed to parse link address: ", linkAddress, ". Error: ", err)
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = linkAddress

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_CREATED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "Created direct link with link address: ", linkAddress)
	} else if channelGid == INDIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Creating TwoSix indirect link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_BIDI)

		var profile multicastProfile
		err := json.Unmarshal([]byte(linkAddress), &profile)
		if err != nil {
			logError(logPrefix, "failed to parse link address: ", linkAddress, ". Error: ", err)
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = linkAddress

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_CREATED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "Created indirect link with link address: ", linkAddress)
	} else {
		logError(logPrefix, "invalid channel GID")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	logDebug("%v returned", logPrefix)
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) LoadLinkAddress(handle uint64, channelGid string, linkAddress string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("LoadLinkAddress: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called")

	if status, ok := plugin.channelStatuses[channelGid]; !ok || status != commsShims.CHANNEL_AVAILABLE {
		logError(logPrefix, "channel not available")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	linkId := plugin.sdk.GenerateLinkId(channelGid)
	if linkId == "" {
		logError("LoadLinkAddress: SDK failed to generate link ID. Is th channel GID valid? ", channelGid)
		return commsShims.PLUGIN_ERROR
	}

	linkProps, err := getDefaultLinkPropertiesForChannel(plugin.sdk, channelGid)
	if err != nil {
		logError(logPrefix, "failed to get default channel properties: ", err)
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	if channelGid == DIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Loading TwoSix direct link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_SEND)

		var profile unicastProfile
		err := json.Unmarshal([]byte(linkAddress), &profile)
		if err != nil {
			logError(logPrefix, "failed to parse link address: ", linkAddress, ". Error: ", err)
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = linkAddress

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_LOADED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "Loaded direct link with link address: ", linkAddress)
	} else if channelGid == INDIRECT_CHANNEL_GID {
		logDebug(logPrefix, "Loading TwoSix indirect link with ID: ", linkId)

		linkProps.SetLinkType(commsShims.LT_BIDI)

		var profile multicastProfile
		err := json.Unmarshal([]byte(linkAddress), &profile)
		if err != nil {
			logError(logPrefix, "failed to parse link address: ", linkAddress, ". Error: ", err)
			plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
			return commsShims.PLUGIN_ERROR
		}

		plugin.linkProperties[linkId] = linkProps
		plugin.linkProfiles[linkId] = linkAddress

		plugin.sdk.OnLinkStatusChanged(handle, linkId, commsShims.LINK_LOADED, linkProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.UpdateLinkProperties(linkId, linkProps, commsShims.GetRACE_BLOCKING())

		logDebug(logPrefix, "Loaded indirect link with link address: ", linkAddress)
	} else {
		logError(logPrefix, "invalid channel GID")
		plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
		return commsShims.PLUGIN_ERROR
	}

	logDebug("%v returned", logPrefix)
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) LoadLinkAddresses(handle uint64, channelGid string, linkAddresses commsShims.StringVector) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("LoadLinkAddress: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called with link addresses: ", linkAddresses)
	logError(logPrefix, "API not supported for any TwoSix channels")
	plugin.sdk.OnLinkStatusChanged(handle, "", commsShims.LINK_DESTROYED, commsShims.NewLinkProperties(), commsShims.GetRACE_BLOCKING())
	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_ERROR
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) ActivateChannel(handle uint64, channelGid string, roleName string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("ActivateChannel: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called")

	status, ok := plugin.channelStatuses[channelGid]
	if !ok {
		logError(logPrefix, "unknown channel GID")
		return commsShims.PLUGIN_ERROR
	}

	if status == commsShims.CHANNEL_AVAILABLE {
		return commsShims.PLUGIN_OK
	}

	if channelGid == DIRECT_CHANNEL_GID {
		plugin.channelStatuses[channelGid] = commsShims.CHANNEL_STARTING

		response := plugin.sdk.RequestCommonUserInput("hostname")
		if response.GetStatus() != commsShims.SDK_OK {
			logError("Failed to request hostname from user, direct channel cannot be used")
			plugin.channelStatuses[DIRECT_CHANNEL_GID] = commsShims.CHANNEL_FAILED
			channelProps := getDefaultChannelPropertiesForChannel(plugin.sdk, DIRECT_CHANNEL_GID)
			plugin.sdk.OnChannelStatusChanged(
				commsShims.GetNULL_RACE_HANDLE(),
				DIRECT_CHANNEL_GID,
				commsShims.CHANNEL_FAILED,
				channelProps,
				commsShims.GetRACE_BLOCKING(),
			)
			// Don't continue
			return commsShims.PLUGIN_OK
		}
		plugin.requestHostnameHandle = response.GetHandle()

		response = plugin.sdk.RequestPluginUserInput("startPort", "What is the first available port?", true)
		if response.GetStatus() != commsShims.SDK_OK {
			logWarning("Failed to request start port from user")
		}
		plugin.requestStartPortHandle = response.GetHandle()

	} else if channelGid == INDIRECT_CHANNEL_GID {
		plugin.channelStatuses[channelGid] = commsShims.CHANNEL_AVAILABLE
		channelProps := getDefaultChannelPropertiesForChannel(plugin.sdk, channelGid)
		plugin.sdk.OnChannelStatusChanged(handle, channelGid, commsShims.CHANNEL_AVAILABLE, channelProps, commsShims.GetRACE_BLOCKING())
		plugin.sdk.DisplayInfoToUser(fmt.Sprintf("%v is available", INDIRECT_CHANNEL_GID), commsShims.UD_TOAST)
	}

	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) DeactivateChannel(handle uint64, channelGid string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("DeactivateChannel: (handle: %v, channel GID: %v): ", handle, channelGid)
	logDebug(logPrefix, "called")

	status, ok := plugin.channelStatuses[channelGid]
	if !ok {
		logError(logPrefix, "unknown channel GID")
		return commsShims.PLUGIN_ERROR
	}

	plugin.channelStatuses[channelGid] = commsShims.CHANNEL_UNAVAILABLE
	plugin.sdk.OnChannelStatusChanged(handle, channelGid, commsShims.CHANNEL_UNAVAILABLE, commsShims.NewChannelProperties(), commsShims.GetRACE_BLOCKING())

	if status == commsShims.CHANNEL_UNAVAILABLE {
		return commsShims.PLUGIN_OK
	}

	linkIdsToDestroy := []string{}
	for linkId, linkProps := range plugin.linkProperties {
		if linkProps.GetChannelGid() == channelGid {
			linkIdsToDestroy = append(linkIdsToDestroy, linkId)
		}
	}

	for _, linkId := range linkIdsToDestroy {
		// Calls OnLinkStatusChanged to notify SDK that links have been destroyed and call OnConnectionStatusChanged to notify all connnections in each link have been destroyed.
		plugin.DestroyLink(handle, linkId)
	}

	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) OnUserInputReceived(handle uint64, answered bool, response string) commsShims.PluginResponse {
	logPrefix := fmt.Sprintf("OnUserInputReceived: (handle: %v): ", handle)
	logDebug(logPrefix, "called")

	if handle == plugin.requestHostnameHandle {
		if answered {
			plugin.hostname = response
			logInfo(logPrefix, "using hostname ", plugin.hostname)
		} else {
			logError(logPrefix, "direct channel not available without the hostname")
			plugin.channelStatuses[DIRECT_CHANNEL_GID] = commsShims.CHANNEL_DISABLED
			channelProps := getDefaultChannelPropertiesForChannel(plugin.sdk, DIRECT_CHANNEL_GID)
			plugin.sdk.OnChannelStatusChanged(
				commsShims.GetNULL_RACE_HANDLE(),
				DIRECT_CHANNEL_GID,
				commsShims.CHANNEL_DISABLED,
				channelProps,
				commsShims.GetRACE_BLOCKING(),
			)
			// Do not continue handling input
			return commsShims.PLUGIN_OK
		}

		plugin.requestHostnameHandle = 0
	} else if handle == plugin.requestStartPortHandle {
		if answered {
			port, err := strconv.Atoi(response)
			if err != nil {
				logWarning(logPrefix, "error parsing start port, ", response)
			} else {
				plugin.nextAvailablePort = port
				logInfo(logPrefix, "using start port ", plugin.nextAvailablePort)
			}
		} else {
			logWarning(logPrefix, "no answer, using default start port")
		}

		plugin.requestStartPortHandle = 0
	} else {
		logWarning(logPrefix, "handle is not recognized")
		return commsShims.PLUGIN_ERROR
	}

	// Check if all requests have been fulfilled
	if plugin.requestHostnameHandle == 0 && plugin.requestStartPortHandle == 0 {
		plugin.channelStatuses[DIRECT_CHANNEL_GID] = commsShims.CHANNEL_AVAILABLE
		channelProps := getDefaultChannelPropertiesForChannel(plugin.sdk, DIRECT_CHANNEL_GID)
		plugin.sdk.OnChannelStatusChanged(
			commsShims.GetNULL_RACE_HANDLE(),
			DIRECT_CHANNEL_GID,
			commsShims.CHANNEL_AVAILABLE,
			channelProps,
			commsShims.GetRACE_BLOCKING(),
		)
		plugin.sdk.DisplayInfoToUser(fmt.Sprintf("%v is available", DIRECT_CHANNEL_GID), commsShims.UD_TOAST)
	}

	logDebug(logPrefix, "returned")
	return commsShims.PLUGIN_OK
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) FlushChannel(handle uint64, channelGid string, batchId uint64) commsShims.PluginResponse {
	logError("FlushChannel: plugin does not support flushing")
	return commsShims.PLUGIN_ERROR
}

func (plugin *overwrittenMethodsOnCommsPluginTwoSix) OnUserAcknowledgementReceived(handle uint64) commsShims.PluginResponse {
	logDebug("OnUserAcknowledgementReceived: called")
	return commsShims.PLUGIN_OK
}

// TODO: this wrapper function is used for convenience until a SWIG typemap is created for std::vector<std::string> to []string.
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) raceSdkReceiveEncPkgWrapper(encPkg commsShims.EncPkg, connectionIds []string) {
	connectionIdsVector := commsShims.NewStringVector()
	defer commsShims.DeleteStringVector(connectionIdsVector)
	for _, persona := range connectionIds {
		connectionIdsVector.Add(persona)
	}

	// Send EncPkg to the SDK for processing
	response := plugin.sdk.ReceiveEncPkg(encPkg, connectionIdsVector, commsShims.GetRACE_BLOCKING())

	// Handle Success/Failure
	responseStatus := response.GetStatus()
	if responseStatus != commsShims.SDK_OK {
		// TODO better handling of failure to receive EncPkg
		logError("Failed sending encPkg for connections ", connectionIdsVector.Size(), " to the SDK: ", responseStatus)
	}
}

// TODO
func (plugin *overwrittenMethodsOnCommsPluginTwoSix) connectionMonitor(connection CommsConn) {
	logInfo("connectionMonitor: called")
	defer logInfo("connectionMonitor: returned")
	connection.Receive(plugin)
	logInfo("connectionMonitor: Shutting down")
	plugin.recvChannel <- 1
}

var plugin *overwrittenMethodsOnCommsPluginTwoSix = nil

// TODO
func InitCommsPluginTwoSix(sdk uintptr) {
	logInfo("InitCommsPluginTwoSix: called")
	if plugin != nil {
		logWarning("Trying to construct a new Golang plugin when one has been created already")
		return
	}

	plugin = &overwrittenMethodsOnCommsPluginTwoSix{}
	plugin.sdk = commsShims.SwigcptrIRaceSdkComms(sdk)

	logInfo("InitCommsPluginTwoSix: returned")
}

//export CreatePluginCommsGolang
func CreatePluginCommsGolang(sdk uintptr) {
	logInfo("CreatePluginCommsGolang: called")
	InitCommsPluginTwoSix(sdk)
	logInfo("CreatePluginCommsGolang: returned")
}

//export DestroyPluginCommsGolang
func DestroyPluginCommsGolang() {
	logInfo("DestroyPluginCommsGolang: called")
	if plugin != nil {
		plugin = nil
	}
	logInfo("DestroyPluginCommsGolang: returned")
}

// For some reason, commsShims.PluginResponse, etc. are not recognized as exportable types
type PluginResponse int
type LinkType int

// Swig didn't bother to export this function, so here it is, copied straight from
// commsPluginBindingsGolang.go all its glory (or should I say... gory). We need this
// in order to properly free memory allocated by C++.
type swig_gostring struct {
	p uintptr
	n int
}

func swigCopyString(s string) string {
	p := *(*swig_gostring)(unsafe.Pointer(&s))
	r := string((*[0x7fffffff]byte)(unsafe.Pointer(p.p))[:p.n])
	commsShims.Swig_free(p.p)
	return r
}

//export PluginCommsGolangInit
func PluginCommsGolangInit(pluginConfig uintptr) PluginResponse {
	return PluginResponse(plugin.Init(commsShims.SwigcptrPluginConfig(pluginConfig)))
}

//export PluginCommsGolangShutdown
func PluginCommsGolangShutdown() PluginResponse {
	return PluginResponse(plugin.Shutdown())
}

//export PluginCommsGolangSendPackage
func PluginCommsGolangSendPackage(handle uint64, connectionId string, encPkg uintptr, timeoutTimestamp float64, batchId uint64) PluginResponse {
	return PluginResponse(plugin.SendPackage(handle, swigCopyString(connectionId), commsShims.SwigcptrEncPkg(encPkg), timeoutTimestamp, batchId))
}

//export PluginCommsGolangOpenConnection
func PluginCommsGolangOpenConnection(handle uint64, linkType LinkType, linkId string, link_hints string, send_timeout int) PluginResponse {
	return PluginResponse(plugin.OpenConnection(handle, commsShims.LinkType(linkType), swigCopyString(linkId), link_hints, send_timeout))
}

//export PluginCommsGolangCloseConnection
func PluginCommsGolangCloseConnection(handle uint64, connectionId string) PluginResponse {
	return PluginResponse(plugin.CloseConnection(handle, swigCopyString(connectionId)))
}

//export PluginCommsGolangDestroyLink
func PluginCommsGolangDestroyLink(handle uint64, linkId string) PluginResponse {
	return PluginResponse(plugin.DestroyLink(handle, swigCopyString(linkId)))
}

//export PluginCommsGolangCreateLink
func PluginCommsGolangCreateLink(handle uint64, channelGid string) PluginResponse {
	return PluginResponse(plugin.CreateLink(handle, swigCopyString(channelGid)))
}

//export PluginCommsGolangCreateLinkFromAddress
func PluginCommsGolangCreateLinkFromAddress(handle uint64, channelGid string, linkAddress string) PluginResponse {
	return PluginResponse(plugin.CreateLinkFromAddress(handle, swigCopyString(channelGid), swigCopyString(linkAddress)))
}

//export PluginCommsGolangLoadLinkAddress
func PluginCommsGolangLoadLinkAddress(handle uint64, channelGid string, linkAddress string) PluginResponse {
	return PluginResponse(plugin.LoadLinkAddress(handle, swigCopyString(channelGid), swigCopyString(linkAddress)))
}

//export PluginCommsGolangLoadLinkAddresses
func PluginCommsGolangLoadLinkAddresses(handle uint64, channelGid string, linkAddresses uintptr) PluginResponse {
	return PluginResponse(plugin.LoadLinkAddresses(handle, swigCopyString(channelGid), commsShims.SwigcptrStringVector(linkAddresses)))
}

//export PluginCommsGolangDeactivateChannel
func PluginCommsGolangDeactivateChannel(handle uint64, channelGid string) PluginResponse {
	return PluginResponse(plugin.DeactivateChannel(handle, swigCopyString(channelGid)))
}

//export PluginCommsGolangActivateChannel
func PluginCommsGolangActivateChannel(handle uint64, channelGid string, roleName string) PluginResponse {
	return PluginResponse(plugin.ActivateChannel(handle, swigCopyString(channelGid), swigCopyString(roleName)))
}

//export PluginCommsGolangOnUserInputReceived
func PluginCommsGolangOnUserInputReceived(handle uint64, answered bool, response string) PluginResponse {
	return PluginResponse(plugin.OnUserInputReceived(handle, answered, swigCopyString(response)))
}

//export PluginCommsGolangFlushChannel
func PluginCommsGolangFlushChannel(handle uint64, connId string, batchId uint64) PluginResponse {
	return PluginResponse(plugin.FlushChannel(handle, swigCopyString(connId), batchId))
}

//export PluginCommsGolangOnUserAcknowledgementReceived
func PluginCommsGolangOnUserAcknowledgementReceived(handle uint64) PluginResponse {
	return PluginResponse(plugin.OnUserAcknowledgementReceived(handle))
}

// TODO
func main() {}
