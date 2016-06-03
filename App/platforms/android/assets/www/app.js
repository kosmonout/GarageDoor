//
// Copyright 2015, Evothings AB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
 
 
 //////////////////////////////// Init
$(document).ready(function () {
	
	$('#connectButton').click(function () {
		app.connect();
	})
	
	$('#GarageDoorButton').click(function () {
		navigator.vibrate(300);
		app.Garage();
	})
	
	$('#GateDoorButton').click(function () {
		navigator.vibrate(300);
		app.Poort();
	})
	
	$('#LightsOnButton').click(function () {
		navigator.vibrate(300);
		app.Lights();
	})
})
var app = {}
var TimerVar;
app.PORT = 1337
app.socketId

$(window).load(function(){  
	console.log('Load Window connect');
	document.addEventListener('pause', function(){
		clearTimeout(TimerVar); 
		chrome.sockets.tcp.close(app.socketId);
		chrome.sockets.tcp.disconnect(app.socketId); 
		console.log('page hide');
	},false)
	document.addEventListener('resume', function(){
		console.log('page show');
		clearTimeout(TimerVar); 
		chrome.sockets.tcp.close(app.socketId);
		chrome.sockets.tcp.disconnect(app.socketId); 
		app.connect();
	},false)
	app.connect(); 
});
   


	app.connect = function () {

	var IPAddress = $('#IPAddress').val()

		console.log('Trying to connect to ' + IPAddress)

		$('#startView').hide() 
		$('#connectingStatus').text('Connecting to ' + IPAddress)
		$('#connectingView').show()
		

		chrome.sockets.tcp.create(function (createInfo) {
			app.socketId = createInfo.socketId;
				chrome.sockets.tcp.connect(
					app.socketId,
					IPAddress,
					app.PORT,
					connectedCallback)
		})
		
		
		chrome.sockets.tcp.onReceive.addListener(function(info) { app.receivedData(info.data); });
		chrome.sockets.tcp.setPaused(false);

	function connectedCallback(result) { 

		if (result === 0) {
			console.log('Connected to ' + IPAddress);
			$('#connectingView').hide(); 
			$('#controlView').show();
			clearTimeout(TimerVar);
			app.sendString('Z'); //Connect client with a placebo send string
			 TimerVar=setTimeout(function(){ 
			 console.log('Connection Lost Callback')
			 chrome.sockets.tcp.close(app.socketId);
			 chrome.sockets.tcp.disconnect(app.socketId); 
			 app.connect()
			}, 3000);
			app.receivedData

		} else {

			var errorMessage = 'Failed to connect to ' + app.IPAdress
				console.log(errorMessage)
				//navigator.notification.alert(errorMessage, function () {})
				clearTimeout(TimerVar);
				chrome.sockets.tcp.close(app.socketId);
				chrome.sockets.tcp.disconnect(app.socketId);  
				app.connect();
		   		//$('#connectingView').hide() 
				//$('#startView').show()
		}
	}
}

app.sendString = function (sendString) { 
	console.log('Trying to send:' + sendString)
	chrome.sockets.tcp.getInfo(
		app.socketId,
		function(res) {
			console.log('Resultinfo:' +res.localAddress);
		}
	)
	chrome.sockets.tcp.send(
		app.socketId,
		app.stringToBuffer(sendString),
		function (sendInfo) {
			console.log('Result:' + sendInfo.resultCode)
			if (sendInfo.resultCode < 0) {
				console.log('Connection Lost Send');
				clearTimeout(TimerVar);
				chrome.sockets.tcp.close(app.socketId);
				chrome.sockets.tcp.disconnect(app.socketId); 
				//var errorMessage = 'Failed to send data'
			//	console.log(errorMessage);
				//navigator.notification.alert(errorMessage, function () {})
				app.connect();
			}
		}
	)
}

app.receivedData = function (data) {
	
		var data = new Uint8Array(data);
		console.log('Data received: [' + data[0] + ', ' + data[1] + ', ' + data[2] + ']');
		clearTimeout(TimerVar);
		TimerVar=setTimeout(function(){ 
			console.log('Connection Lost data');
			chrome.sockets.tcp.close(app.socketId);
			chrome.sockets.tcp.disconnect(app.socketId); 
			app.connect();
		}, 3000);
		if (data[0] === 65) { //ASCII "A" Lichten aan
			$('#LightsOnStatus').text('Lichten AAN');
		} 
		if (data[0] === 85) { //ASCII "U" Lichten uit
			$('#LightsOnStatus').text('Lichten UIT');
		}
		if (data[0] === 79) { //ASCII "O" Openen garage deur
			$('#GarageDoorStatus').text('Openen...');
		} 
		if (data[0] === 83) { //ASCII "S" Sluiten garage deur
			$('#GarageDoorStatus').text('Sluiten...');
		}
		if (data[0] === 76) { //ASCII "L" Garage deur is los :-)
			$('#GarageDoorStatus').text('Staat open!');
		} 
		if (data[0] === 68) { //ASCII "D" Garage deur is dicht
			$('#GarageDoorStatus').text('Gesloten.');
		}
		if (data[0] === 71) { //ASCII "G" Poort deur is aan het openen
			$('#GateDoorStatus').text('Openen...');
		}
		if (data[0] === 78) { //ASCII "N" Poort deur geen actie
			$('#GateDoorStatus').text('......');
		}
};

app.Garage = function () {

	app.sendString('G')

//	$('#led').removeClass('ledOff').addClass('ledOn')

//	$('#led').unbind('click').click(function () {
//		app.ledOff()
	//})
}

app.Poort = function () {

	app.sendString('P')

//	$('#led').removeClass('ledOn').addClass('ledOff')

	//$('#led').unbind('click').click(function () {
	//	app.ledOn()
//	})
}

app.Lights = function () {

	app.sendString('Y')

	//$('#led').removeClass('ledOn').addClass('ledOff')

	//$('#led').unbind('click').click(function () {
	//	app.ledOn()
//	})
}


app.stringToBuffer = function (string) {

	var buffer = new ArrayBuffer(string.length)
		var bufferView = new Uint8Array(buffer)

		for (var i = 0; i < string.length; ++i) {

			bufferView[i] = string.charCodeAt(i)
		}

		return buffer
}

app.bufferToString = function (buffer) {

	return String.fromCharCode.apply(null, new Uint8Array(buffer))
}


