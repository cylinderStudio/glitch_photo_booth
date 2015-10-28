var express = require('express'),
	app = express(),
	http = require('http').Server(app),
	dgram = require('dgram'),
	osc = require('osc-min'),
	AWS = require('aws-sdk'),
	proxy = require('proxy-agent'),
	request = require('request'),
	fs = require('fs'),
	config = require('./creds/config'),
	sendSocket = dgram.createSocket({type:"udp4",reuseAddr:true}),
	receiveSocket = dgram.createSocket({type:"udp4",reuseAddr:true}),
	mediaUUID;

const SEND_PORT = 12345,
	RECEIVE_PORT = 12346,
	OUTPUT_DIR = '../data/',
	MEDIA_TYPE = 'video',				// set to 'photo' for MMS, 'video' for SMS
	STORE_ID = '1',							// store number: '1', '220', etc.
	EXPIRE_TIME = 2592000,			// S3 file expiration, in milliseconds: 1 mo. = 60 * 60 * 24 * 30 = 2592000
	AWS_PARAMS = config.AWS_params,
	BUCKET = config.settings.bucket,
	API_URL = config.settings.photobooth_gateway_URL,

AWS.config.update(AWS_PARAMS);
AWS.config.update({
  httpOptions: { agent: proxy('http://webproxy.nordstrom.net:8181') }
});

sendSocket.bind(SEND_PORT);
receiveSocket.bind(RECEIVE_PORT);


function getOSCMessage(msg){
	//extract relevant data from OSC Message
	var oscMessage = osc.fromBuffer(msg);
	try {
		// console.log(oscMessage);
		// translate osc buffer into javascript object

		var element = oscMessage.elements[0],
			address = element.address;

		if(address === "/video"){
			var	args = element.args,
			cleanMovie = args[0].value,
			distortedMovie = args[1].value;

			return {
				type: 'video',
				address: address,
				cleanMovie: cleanMovie,
				distortedMovie: distortedMovie
			};
		}else if(address === "/heartbeat"){
			return {
				type: 'heartbeat'
			}
		}

	}catch(error){
		print("invalid OSC packet");
	}
}

receiveSocket.on('message', function(message, remote){
	var oscData = getOSCMessage(message);
<<<<<<< HEAD
	aws_s3.saveMediaOnS3(OUTPUT_DIR + oscData.distortedMovie);

	console.log('filename: ', oscData.distortedMovie);

	/* 
		message: {
		address: '/video',
		cleanMovie: 'filename.mp4',
		distortedMovie: 'filename_distorted.mp4'
=======
	if(oscData.type === 'video'){
		aws_s3.saveMediaOnS3(OUTPUT_DIR + oscData.distortedMovie);
		print('filename: ' + oscData.distortedMovie);
	}else if(oscData.type === 'heartbeat'){
		print('heartbeat received.');
>>>>>>> upstream/master
	}
});

function sendOSCMessage(address, code){
	code = code || '';

	var buffer = osc.toBuffer({
		address: '/' + address,
		args : [{
			type: "string",
			value: code
		}]
	});
	sendSocket.send(buffer, 0, buffer.length, SEND_PORT, '127.0.0.1');
}

var sendInterval = setInterval(function(){
	sendOSCMessage('heartbeat', '1337H4X04L0L0L0L0L');
}, 10000);

// === WATERFALL
/* 	
	Process where each in a series of functions calls the next on success.
	If the process fails at any point in the waterfall, the program calls
	sendOSCMessage('failure'), but if it reaches the end of the process
	successfully, it calls sendOSCMessage(mediaUUID).

	Step 1: aws_s3.saveImageOnS3(image_string)
	Step 2: aws_s3.getAndReturnSignedURL(object_key)
	Step 3: postMetadataToGateway(URL)
*/

var aws_s3 = (function() {

	var s3 = new AWS.S3(); 

	return {
		saveMediaOnS3: function(media_string) {
			fs.readFile(media_string, function (err, data) {
			  if (err) { 
			  	print('error uploading to s3: ' + err);
			  	sendOSCMessage('failure'); 
			  } else {
			  	mediaUUID = generateUUID();		// globar var used in file stamp and msg back to client
			  	var file_stamp = generateFileStamp();
				  var object_key = 'store_' + STORE_ID + '/' + file_stamp + '.mp4';

					var params = {
						Bucket: config.settings.bucket, 
						Key: object_key, 
						Body: data,
					  ContentType: 'video/mp4'
					};

					s3.putObject(params, function(err, data) {
						if (err) { 
							print('Error putting object on S3: ' + err); 
							sendOSCMessage('failure');
						} else { 
							print('Placed object on S3: ' + object_key); 
							aws_s3.getAndReturnSignedURL(object_key);
						}  
					});
			  }
			});
		}, 

		getAndReturnSignedURL: function(object_key) {
			// URL should expire in 
			var params = {
				Bucket: BUCKET,
				Key: object_key,
				Expires: EXPIRE_TIME
			}

			s3.getSignedUrl('getObject', params, function(err, url) {
				if (err) {
					print('Error getting signed URL from S3: ' + err);
					sendOSCMessage('failure');
				} else {
					print('Returned signed URL: ' +  url);
					postMetadataToGateway(url);
				}
			});
		}
	}
})();

function postMetadataToGateway(URL) {
	print('Attempting to save media data in Photo Booth Gateway');

	var tempObject = { 
		'UUID': mediaUUID, 
		'insert_date': new Date(),
		'media': URL,			
		'store_id': 'Store ' + STORE_ID,
		'media_type': MEDIA_TYPE
	};

	request({
    url: API_URL + '/save',
    method: 'POST',
    json: true,
    body: tempObject
	}, function (error, response, body) {
		// print(response);
		if (!error && response.statusCode == 200) {
	    	// print('success: ' + response);
			print('Saved media data in Photo Booth Gateway: ' + mediaUUID);
			sendOSCMessage('uploaded', mediaUUID); 
	  } else {
			print('Failed to save media data in Photo Booth Gateway: ' + response);
			print('Desc: ' + error);
			sendOSCMessage('failure', '');
	  }
	});
}
// === END WATERFALL

// === HELPERS

function generateUUID() { 
	var UUID = '';
  var possible = 'abcdefghijkmnopqrstuvwxyz23456789';

  for (var i=0; i < 4; i++) {
  	UUID += possible.charAt(Math.floor(Math.random() * possible.length));
  }

  return UUID;
}

function generateFileStamp(UUID) {
	var date = new Date();

	var month = addStringDigit((1 + date.getMonth()).toString());
	var day = addStringDigit(date.getDate().toString());
	var hours = addStringDigit(date.getHours().toString());
	var minutes = addStringDigit(date.getMinutes().toString());

	var file_stamp = STORE_ID.toString() + month.toString() + day.toString() + hours.toString() + minutes.toString() + '_' + mediaUUID;

	function addStringDigit(tempString) {
		if (tempString.length === 1) { tempString = '0' + tempString; }
		return tempString;
	}

	return file_stamp;
}

function print(message){
	console.log(Date() + ': ' + message);
}

// === END HELPERS

print("Server started");
