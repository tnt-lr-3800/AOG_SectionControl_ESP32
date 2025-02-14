// WIFI handling 7. Maerz 2021 for ESP32  -------------------------------------------

void WiFi_handle_connection(void* pvParameters) {
    task_WiFiConnectRunning = true;
    if (Set.DataTransVia > 10) { vTaskDelay(5000); } //start Ethernet first, if needed for data transfer
    for (;;) {
        if (WiFi_connect_step == 0) {
            task_WiFiConnectRunning = false;
            if (Set.debugmode) { Serial.println("closing WiFi connection task"); }
            delay(1);
            vTaskDelete(NULL);
            delay(1);
        }
        else {
            vTaskDelay(500);//do every half second

            now = millis();

            IPAddress gwIP, myIP;

            if (Set.debugmode) { Serial.print("WiFi_connect_step: "); Serial.println(WiFi_connect_step); }
            switch (WiFi_connect_step) {
            case 1:
                //check WiFi
                if (Ping.ping(Set.WiFi_gwip)) { //WiFi is available, retry to connect NTRIP
                //    Ntrip_restart = 1;
                    WiFi_connect_step = 0;
                    /*      if ((Set.NtripClientBy == 2) && (!task_NTRIP_Client_running)) {
                              {
                                  xTaskCreatePinnedToCore(NTRIP_Client_Code, "Core1", 3072, NULL, 1, &taskHandle_WiFi_NTRIP, 1);
                                  delay(500);
                              }
                          }*/
                }
                else { WiFi_connect_step = 4; }//no network
                break;
                //close Webserver, UDP ...
            case 4:
                WiFi_netw_nr = 0;
                if (WebIORunning) {
                    WiFi_Server.close();
                    WebIORunning = false;
                }
                WiFiUDPRunning = false;
                WiFi_connect_step++;
                break;
                //turn WiFi off
            case 5:
                WiFi.mode(WIFI_OFF);
                WiFi_network_search_timeout = 0;
                WiFi_connect_step = 10;
                break;

                //WiFi network scan
            case 10:
                WiFi_netw_nr = 0;
                WebIORunning = false;
                WiFiUDPRunning = false;
                if (WiFi_network_search_timeout == 0) {   //first run                 
                    WiFi_network_search_timeout = now + (Set.timeoutRouter * 1000);
                } 
                WiFi_scan_networks();
                //timeout?
                if (now > WiFi_network_search_timeout) { WiFi_connect_step = 50; }
                else {
                    if (WiFi_netw_nr > 0) {
                        //found network
                        WiFi_connect_step++;
                        WiFi_network_search_timeout = 0;//reset timer
                    }
                }
                break;
                //start WiFi connection
            case 11:
                WiFi.mode(WIFI_STA);   //  Workstation  
                WiFi_connect_step++;
                break;
            case 12:
                if (WiFi_network_search_timeout == 0) {   //first run  
                    WiFi_network_search_timeout = now + (Set.timeoutRouter * 500);//half time
                }
                WiFi_STA_connect_network();
                WiFi_connect_step++;
                break;
            case 13:
                if (WiFi.status() != WL_CONNECTED) {
                    Serial.print(".");
                    if (now > WiFi_network_search_timeout) {
                        //timeout
                        WiFi_STA_connect_call_nr++;
                        WiFi_connect_step = 17;//close WiFi and try again
                        WiFi_network_search_timeout += (Set.timeoutRouter * 500);//add rest of time
                    }
                }
                else {
                    //connected
                    WiFi_connect_step++;
                    WiFi_network_search_timeout = 0;//reset timer
                }
                break;
                //change IP / DHCP
            case 14:
                //connected
                Serial.println();
                Serial.println("WiFi Client successfully connected");
                Serial.print("Connected IP - Address : ");
                myIP = WiFi.localIP();
                Serial.println(myIP);
                //after connecting get IP from router -> change it to x.x.x.IP Ending (from settings)
                myIP[3] = Set.WiFi_myip[3]; //set ESP32 IP to x.x.x.myIP_ending
                Serial.print("changing IP to: ");
                Serial.println(myIP);
                gwIP = WiFi.gatewayIP();
                if (!WiFi.config(myIP, gwIP, Set.mask, gwIP)) { Serial.println("STA Failed to configure"); }
                WiFi_connect_step++;
                break;
            case 15:
                myIP = WiFi.localIP();
                Serial.print("Connected IP - Address : "); Serial.println(myIP);
                WiFi_ipDestination = myIP;
                WiFi_ipDestination[3] = Set.WiFi_ipDest_ending;
                Serial.print("sending to IP - Address : "); Serial.println(WiFi_ipDestination);
                gwIP = WiFi.gatewayIP();
                Serial.print("Gateway IP - Address : "); Serial.println(gwIP);
                my_WiFi_Mode = 1;// WIFI_STA;
                WiFi_connect_step = 20;
                break;
                //no connection at first try, try again
            case 17:
                if (WiFi_STA_connect_call_nr > 2) { //create access point
                    WiFi_connect_step = 50;
                    WiFi_netw_nr = 0;
                }
                else {
                    WiFi.disconnect();
                    vTaskDelay(2);
                    WiFi_connect_step++;
                    Serial.print("-");
                }
                break;
            case 18:
                WiFi.mode(WIFI_OFF); vTaskDelay(2);
                WiFi_connect_step = 11; //set STA
                break;

                //UDP
            case 20://init WiFi UDP listening to AOG
                if (WiFiUDPFromAOG.listen(Set.PortFromAOG)) {
                    Serial.print("UDP writing to IP: ");
                    Serial.println(WiFi_ipDestination);
                    Serial.print("UDP writing to port: ");
                    Serial.println(Set.PortDestination);
                    Serial.print("UDP writing from port: ");
                    Serial.println(Set.PortSCToAOG);
                    Serial.print("UDP listening to AOG port: ");
                    Serial.println(Set.PortFromAOG);
                }
                else { Serial.println("Error starting WiFi UDP "); }
                WiFi_connect_step++;
                break;
            case 21:
                // UDP message from AgIO packet handling
                WiFiUDPFromAOG.onPacket([](AsyncUDPPacket packet)
                    {//write data into array
                        unsigned int packetLength;
                        byte nextincommingBytesArrayNr = (incommingBytesArrayNr + 1) % incommingDataArraySize;
                        for (unsigned int i = 0; i < packet.length(); i++) {
                            incommingBytes[nextincommingBytesArrayNr][i] = packet.data()[i];
                        }
                        incommingDataLength[nextincommingBytesArrayNr] = packet.length();
                        incommingBytesArrayNr = nextincommingBytesArrayNr;
                    });  // end of onPacket call
                WiFiUDPRunning = true;
                WiFi_connect_step = 100;
                break;

                //Access point start
            case 50://start access point
                WiFi_Start_AP();
                WiFi_connect_step++;
                break;
            case 51:
                if (my_WiFi_Mode == 2) { WiFi_connect_step++; }
                break;
            case 52://init WiFi UDP listening to AOG
                if (WiFiUDPToAOG.listen(Set.PortSCToAOG)) {
                    Serial.print("UDP writing to IP: ");
                    Serial.println(WiFi_ipDestination);
                    Serial.print("UDP writing to port: ");
                    Serial.println(Set.PortDestination);
                    Serial.print("UDP writing from port: ");
                    Serial.println(Set.PortSCToAOG);
                    Serial.print("UDP listening to port: ");
                    Serial.println(Set.PortFromAOG);
                }
                else { Serial.println("Error starting WiFi UDP"); }
                WiFi_connect_step++;
                break;
            case 53:
                // UDP message from AgIO packet handling
                WiFiUDPFromAOG.onPacket([](AsyncUDPPacket packet)
                    {//write data into array
                        unsigned int packetLength;
                        byte nextincommingBytesArrayNr = (incommingBytesArrayNr + 1) % incommingDataArraySize;
                        for (unsigned int i = 0; i < packet.length(); i++) {
                            incommingBytes[nextincommingBytesArrayNr][i] = packet.data()[i];
                        }
                        incommingDataLength[nextincommingBytesArrayNr] = packet.length();
                        incommingBytesArrayNr = nextincommingBytesArrayNr;
                    });  // end of onPacket call
                WiFiUDPRunning = true;
                WiFi_connect_step = 100;
                break;

                //Webserver start
            case 100:
                //start Server for Webinterface
                WiFiStartServer();
                WiFi_connect_step++;
                break;

            case 101:
                WebIOTimeOut = millis() + (long(Set.timeoutWebIO) * 60000);
                xTaskCreate(doWebinterface, "WebIOHandle", 5000, NULL, 1, &taskHandle_WebIO);
                delay(300);                
                WiFi_connect_step = 0;
                LED_WIFI_ON = true;
                Serial.println(); Serial.println();
                if (WiFi_netw_nr == 0) { myIP = WiFi.softAPIP(); }
                else {
                    myIP = WiFi.localIP();
                }

                Serial.print("started settings Webinterface at: ");
                for (byte i = 0; i < 3; i++) {
                    Serial.print(myIP[i]); Serial.print(".");
                }
                Serial.println(myIP[3]);
                Serial.println("type IP in Internet browser to get to webinterface");
                Serial.print("you need to be in WiFi network ");
                switch (WiFi_netw_nr) {
                case 0: Serial.print(Set.ssid_ap); break;
                case 1: Serial.print(Set.ssid1); break;
                }
                Serial.println(" to get access"); Serial.println(); Serial.println();
#if useLED_BUILTIN
                digitalWrite(LED_BUILTIN, HIGH);
#endif
                digitalWrite(Set.LEDWiFi_PIN, Set.LEDWiFi_ON_Level);
                break;

            default:
                WiFi_connect_step++;
                Serial.print("default called at WiFi_connection_step "); Serial.println(WiFi_connect_step);
                break;
            }
        }
    }
}

//---------------------------------------------------------------------
// scanning for known WiFi networks

void WiFi_scan_networks()
{
    Serial.println("scanning for WiFi networks");
    // WiFi.scanNetworks will return the number of networks found
    int WiFi_num_netw_inReach = WiFi.scanNetworks();
    Serial.print("scan done: ");
    if (WiFi_num_netw_inReach <= 0) {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(WiFi_num_netw_inReach);
        Serial.println(" network(s) found");
        for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
            Serial.println("#" + String(i + 1) + " network : " + WiFi.SSID(i));
        }
        delay(800);//.SSID gives no value if no delay
        delay(500);

        for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
            if (WiFi.SSID(i) == Set.ssid1) {
                // network found in list
                Serial.print("Connecting to: "); Serial.println(Set.ssid1);
                WiFi_netw_nr = 1;
                break;
            }
        }
        if ((WiFi_netw_nr == 0) && (Set.ssid2 != "")) {
            for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
                if (WiFi.SSID(i) == Set.ssid2) {
                    // network found in list
                    Serial.print("Connecting to: "); Serial.println(Set.ssid2);
                    WiFi_netw_nr = 2;
                    break;
                }
            }
        }
        if ((WiFi_netw_nr == 0) && (Set.ssid3 != "")) {
            for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
                if (WiFi.SSID(i) == Set.ssid3) {
                    // network found in list
                    Serial.print("Connecting to: "); Serial.println(Set.ssid3);
                    WiFi_netw_nr = 3;
                    break;
                }
            }
        }
        if ((WiFi_netw_nr == 0) && (Set.ssid4 != "")) {
            for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
                if (WiFi.SSID(i) == Set.ssid4) {
                    // network found in list
                    Serial.print("Connecting to: "); Serial.println(Set.ssid4);
                    WiFi_netw_nr = 4;
                    break;
                }
            }
        }
        if ((WiFi_netw_nr == 0) && (Set.ssid5 != "")) {
            for (int i = 0; i < WiFi_num_netw_inReach; ++i) {
                if (WiFi.SSID(i) == Set.ssid5) {
                    // network found in list
                    Serial.print("Connecting to: "); Serial.println(Set.ssid5);
                    WiFi_netw_nr = 5;
                    break;
                }
            }
        }
    }
}  //end WiFi_scan_networks()

//-------------------------------------------------------------------------------------------------
//connects to WiFi network

void WiFi_STA_connect_network() {//run WiFi_scan_networks first
   // Serial.print("netwNr: "); Serial.print(WiFi_netw_nr);
    switch (WiFi_netw_nr) {
    case 1: WiFi.begin(Set.ssid1, Set.password1); break;
    case 2: WiFi.begin(Set.ssid2, Set.password2); break;
    case 3: WiFi.begin(Set.ssid3, Set.password3); break;
    case 4: WiFi.begin(Set.ssid4, Set.password4); break;
    case 5: WiFi.begin(Set.ssid5, Set.password5); break;
    }
    //set IP to DHCP on first run. call immediately after begin
    if (WiFi_STA_connect_call_nr == 0) { WiFi.config(0U, 0U, 0U); Serial.println("enable DHCP for WiFi"); WiFi_STA_connect_call_nr++; }
    delay(2);
}

//-------------------------------------------------------------------------------------------------
// start WiFi Access Point = only if no existing WiFi or connection fails
//ESP32

void WiFi_Start_AP() {
    WiFi.mode(WIFI_AP);   // Accesspoint
    WiFi.softAP(Set.ssid_ap, "");
    while (!SYSTEM_EVENT_AP_START) // wait until AP has started
    {
        delay(100);
        Serial.print(".");
    }
    delay(150);//right IP adress only with this delay 
    WiFi.softAPConfig(Set.WiFi_gwip, Set.WiFi_gwip, Set.mask);  // set fix IP for AP  
    delay(350);
    IPAddress myIP = WiFi.softAPIP();
    delay(300);
    Serial.print("Accesspoint started - Name : ");
    Serial.println(Set.ssid_ap);
    Serial.print(" IP address: ");
    WiFi_ipDestination = myIP;
    Serial.println(WiFi_ipDestination);
    WiFi_ipDestination[3] = 255;
    my_WiFi_Mode = WIFI_AP;
}

//=================================================================================================
//Ethernet handling for ESP32 14. Feb 2021
//-------------------------------------------------------------------------------------------------
void Eth_handle_connection(void* pvParameters) {
    if (Set.timeoutRouter < 20) { if (WiFi_connect_step != 0) { vTaskDelay(25000); } }//waiting for WiFi to start first
    if (Set.debugmode) { Serial.println("started new task: Ethernet handle connection"); }

    for (;;) { // MAIN LOOP
        vTaskDelay(350);
        if (Set.debugmode) { Serial.print("Ethernet connection step: "); Serial.println(Eth_connect_step); }
        if (Eth_connect_step > 0) {
            switch (Eth_connect_step) {
            case 10:
                Ethernet.init(Set.Eth_CS_PIN);
                Eth_connect_step++;
                break;
            case 11:
                if (Set.Eth_static_IP) { Ethernet.begin(Set.Eth_mac, Set.Eth_myip); }
                else {
                    Ethernet.begin(Set.Eth_mac); //use DHCP
                    if (Set.debugmode) { Serial.println("waiting for DHCP IP adress"); }
                }
                Eth_connect_step++;
                break;
            case 12:
                if (Ethernet.hardwareStatus() == EthernetNoHardware) {
                    Serial.println("no Ethernet hardware, Data Transfer set to WiFi");
                    Eth_connect_step = 255;//no Ethernet, end Ethernet
                    if (Set.DataTransVia == 10) {
                        Set.DataTransVia = 7; //change DataTransfer to WiFi                                                            
                        if (EthDataTaskRunning) { vTaskDelete(taskHandle_DataFromAOGEth); delay(5); EthDataTaskRunning = false; }
                    }
                }
                else {
                    Serial.println("Ethernet hardware found, checking for connection");
                    Eth_connect_step++;
                }
                break;
            case 13:
                if (Ethernet.linkStatus() == LinkOFF) {
                    Serial.println("Ethernet cable is not connected. Retrying in 5 Sek.");
                    vTaskDelay(5000);
                }
                else { Serial.println("Ethernet status OK"); Eth_connect_step++; }
                break;
            case 14:
                Serial.print("Got IP ");
                Serial.println(Ethernet.localIP());
                if ((Ethernet.localIP()[0] == 0) && (Ethernet.localIP()[1] == 0) && (Ethernet.localIP()[2] == 0) && (Ethernet.localIP()[3] == 0)) {
                    //got IP 0.0.0.0 = no DCHP so use static IP
                    Set.Eth_static_IP = true;
                }
                //use DHCP but change IP ending (x.x.x.80)
                if (!Set.Eth_static_IP) {
                    for (byte n = 0; n < 3; n++) {
                        Set.Eth_myip[n] = Ethernet.localIP()[n];
                        Eth_ipDestination[n] = Ethernet.localIP()[n];
                    }
                    Eth_ipDestination[3] = 255;
                    Ethernet.setLocalIP(Set.Eth_myip);
                }
                else {//use static IP
                    for (byte n = 0; n < 3; n++) {
                        Eth_ipDestination[n] = Set.Eth_myip[n];
                    }
                    Eth_ipDestination[3] = Set.Eth_ipDest_ending;
                    Ethernet.setLocalIP(Set.Eth_myip);
                }
                Eth_connect_step++;
                break;
            case 15:
                Serial.print("Ethernet IP of Section Control module: "); Serial.println(Ethernet.localIP());
                Serial.print("Ethernet sending to IP: "); Serial.println(Eth_ipDestination);
                //init UPD Port sending to AOG
                if (EthUDPToAOG.begin(Set.PortSCToAOG))
                {
                    Serial.print("Ethernet UDP sending from port: ");
                    Serial.println(Set.PortSCToAOG);
                }
                Eth_connect_step++;
                break;
            case 16:
                //init UPD Port getting Data from AOG
                if (EthUDPFromAOG.begin(Set.PortFromAOG))
                {
                    Serial.print("Ethernet UDP listening to port: ");
                    Serial.println(Set.PortFromAOG);
                    EthUDPRunning = true;
                }
                Eth_connect_step = 0;//done
                break;

            default:
                Eth_connect_step++;
                break;
            }//switch
        }
    }
    if ((Eth_connect_step > 240) || (Eth_connect_step == 0)) {
        Serial.println("closing Ethernet connection task");
        delay(1);
        vTaskDelete(NULL);
        delay(1);
    }
}

//-------------------------------------------------------------------------------------------------
// Server Index Page for OTA update
//-------------------------------------------------------------------------------------------------


const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<head>"
"<title>Firmware updater</title>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0;\" />\r\n""<style>divbox {background-color: lightgrey;width: 200px;border: 5px solid red;padding:10px;margin: 10px;}</style>"
"</head>"
"<body bgcolor=\"#ccff66\">""<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">"
"<h1>ESP firmware update</h1>"
"ver 4.3 - 10. Mai. 2020<br><br>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<br>Create a .bin file with Arduino IDE: Sketch -> Export compiled Binary<br>"
"<br><b>select .bin file to upload</b>"
"<br>"
"<br>"
"<input type='file' name='update'>"
"<input type='submit' value='Update'>"
"</form>"
"<div id='prg'>progress: 0%</div>"
"<script>"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
" $.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!')"
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>";

