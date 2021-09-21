"use strict";

// ----------------------------------------------------------------------------

bindEventHandler("OnResourceStart", thisResource, function(event, resource) {
	setInterval(updatePlayerScoreboardPing, 3000);
});

// ----------------------------------------------------------------------------

function updatePlayerScoreboardPing() {
	let clients = getClients();
	for(let i in clients) {
		clients[i].setData("v.ping", clients[i].ping, true);
	}
}

// ----------------------------------------------------------------------------