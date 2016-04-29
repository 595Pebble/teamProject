var prev = "";
var stockIndex = 0;
var sleep=0;

//Listens for an "appmessage" coming from the watch
Pebble.addEventListener("appmessage",
  function(e) {   
    var index = e.payload.hello_msg;
    if (index === "wakeup") {      
      sendToServer(index);
      sleep=0;
    }
    else if(sleep==0){
      if (index === "temperature") {        
        if (prev === "temperature") {
          index = "average";        
          sendToServer(index);
        } else if (prev === "average") {           
          index = "low";
          sendToServer(index);
        } else if (prev === "low") {
          index = "high";          
          sendToServer(index);
        } else {sendToServer(index);}
        prev = index;        
      } else if (index === "F/C") {       
        sendToServer(index);
      } else if ( index === "standby") {
        sendToServer(index);
        sleep=1;        
      } else if (index === "+") {        
        sendToServer(index);
      } else if (index === "-") {        
        sendToServer(index);
      } else if (index === "tempHistory") {        
        sendToServer(index);
      }
    }
    
  });

// creates an HTTP request and sends it to a server
function sendToServer(index) {
  
	var req = new XMLHttpRequest();
	var ipAddress = "158.130.107.122"; // Hard coded IP address
	var port = "3001"; // Same port specified as argument to server
	var url = "http://" + ipAddress + ":" + port + "/" + index;
	var method = "GET";
	var async = true;

	req.onload = function(e) {
    // see what came back
    var msg = "No response";
    var response = JSON.parse(req.responseText);
    if (response) {
      if (response.name) {
        msg = response.name;
      }
      else msg = "no temperature";
    }
    // sends message back to pebble
    Pebble.sendAppMessage({ "0": msg });
	};

  req.onerror = function() {
    Pebble.sendAppMessage({"0": "Connection to server failed."});
  };

  req.open(method, url, async);
  req.send(null);
}

