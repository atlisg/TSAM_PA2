# TSAM_PA2
HTTP Server

To run the server:
./src/httpd *PORT*

Ways to test the code:

GET REQUEST (with header):
curl -i http://localhost:*PORT*

POST REQUEST (with header):
curl -i -d "content" http://localhost:*PORT*

HEAD REQUEST:
curl -i -X HEAD http://localhost:*PORT*

test page:
curl -i http://localhost:*PORT*/test?arg1=foo&arg2=bar

color page:
curl -i http://localhost:*PORT*/color?bg=red
