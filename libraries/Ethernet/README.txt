Library modified on 06/07/2012 by PL

added function to retrieve RemoteIP


http://arduino.cc/forum/index.php/topic,82416.msg619420.html#msg619420




I added the following lines to the end of the EthernetClient.cpp file:

uint8_t *EthernetClient::getRemoteIP(uint8_t remoteIP[])
{
  W5100.readSnDIPR(_sock, remoteIP);
  return remoteIP;
}

I then added the following line (under the virtual void stop(); line)to the EthernetClient.h file:
uint8_t *getRemoteIP(uint8_t RemoteIP[]);//adds remote ip address

Finally I used the following code in my sketch to access the remote IP:
client.getRemoteIP(rip); // where rip is defined as byte rip[] = {0,0,0,0 };

to display the IP in the serial monitor, I used:

for (int bcount= 0; bcount < 4; bcount++)
     { 
        Serial.print(rip[bcount], DEC); 
        if (bcount<3) Serial.print(".");
     } 

I'm not sure that this is the most elegant way, but it works in the IDE v1.0