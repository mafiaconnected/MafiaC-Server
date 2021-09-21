"use strict";

// ----------------------------------------------------------------------------

let playerColours = [
	toColour(51, 153, 255, 255),
	toColour(252, 145, 58, 255),
	toColour(186, 85, 211, 255),
	toColour(144, 255, 96, 255),
	toColour(255, 252, 127, 255),
	toColour(131, 189, 209, 255),
	toColour(237, 67, 55, 255),
	toColour(255, 188, 218, 225),
];

// ----------------------------------------------------------------------------

addEventHandler("OnPlayerJoined", function(event, client) {
	client.setData("v.colour", playerColours[client.index], true);
});

// ----------------------------------------------------------------------------

bindEventHandler("OnResourceStart", thisResource, function(event, resource) {
	let clients = getClients();
	for(let i in clients) {
		clients[i].setData("v.colour", playerColours[clients[i].index], true);
	}
	triggerNetworkEvent("v.colour", null);
});

// ----------------------------------------------------------------------------

bindEventHandler("OnResourceStop", thisResource, function(event, resource) {
	let clients = getClients();
	for(let i in clients) {
		clients[i].removeData("v.colour");
	}
	triggerNetworkEvent("v.colour", null);
});

// ----------------------------------------------------------------------------

function getRandomColour() {
	return toColour(getRandom(32, 255), getRandom(32, 255), getRandom(32, 255), 255);
}

// ----------------------------------------------------------------------------

function getRandom(min, max) {
	return Math.floor(Math.random() * (Number(max) - Number(min) + 1)) + Number(min)
}

// ----------------------------------------------------------------------------