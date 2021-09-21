"use strict";

// ----------------------------------------------------------------------------

// Configuration
let titleFont = null;
let listFont = null;

let listWidth = game.width/4;

// ----------------------------------------------------------------------------

bindEventHandler("OnResourceReady", thisResource, function(event, resource) {
	listFont = lucasFont.createDefaultFont(12.0, "Roboto", "Light");

	let fontStream = openFile("pricedown.ttf");
	if(fontStream != null) {
		titleFont = lucasFont.createFont(fontStream, 22.0);
		fontStream.close();
	}
});

// ----------------------------------------------------------------------------

addEventHandler("OnDrawnHUD", function (event) {
	if(isKeyDown(SDLK_TAB)) {
		if(listFont != null && titleFont != null) {
			let scoreboardStart = (game.height/2)-(Math.floor(getClients().length/2)*20);

			titleFont.render("PLAYERS", [game.width/2, scoreboardStart-75], 0, 0.5, 0.0, titleFont.size, COLOUR_WHITE, false, false, false, true);
			titleFont.render("___________________________________", [game.width/2, scoreboardStart-35], 0, 0.5, 0.0, titleFont.size, COLOUR_WHITE, false, false, false, true);

			let listColumns = ["ID", "Name", "Ping"];
			for(let i in listColumns) {
				listFont.render(listColumns[i], [(Math.round(game.width/2)-Math.round(listWidth/listColumns.length))+(Math.round(listWidth/listColumns.length))*i, scoreboardStart-30], 0, 0.5, 0.0, listFont.size, COLOUR_WHITE, false, false, false, true);
			}

			let clients = getClients();
			for(let i in clients) {
				if(!clients[i].console) {
					let colour = clients[i].getData("v.colour") || COLOUR_WHITE;
					let listColumnData = [String(clients[i].index), String(clients[i].name), String(clients[i].getData("v.ping"))];
					for(let j in listColumnData) {
						listFont.render(listColumnData[j], [(Math.round(game.width/2)-Math.round(listWidth/listColumns.length))+(Math.round(listWidth/listColumns.length))*j, scoreboardStart + (i*20)], 0, 0.5, 0.0, listFont.size, COLOUR_WHITE, false, false, false, true);
					}
				}
			}
		}
	}
});

// ----------------------------------------------------------------------------