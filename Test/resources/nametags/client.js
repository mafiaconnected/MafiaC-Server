"use strict";

// ----------------------------------------------------------------------------

// Configuration
let nametagFont = null;
let afkStatusFont = null;
let pingFont = null;
let nametagDistance = 50.0;
let nametagWidth = 70;
let nametagColour = [
    [0,0,0],                    		// (Invalid game)
    [255, 255, 255],            		// Mafia 1
];

// ----------------------------------------------------------------------------

addEventHandler("OnResourceReady", function(event, resource) {
	if (resource == thisResource) {
		nametagFont = lucasFont.createDefaultFont(12.0, "Roboto", "Light");
		afkStatusFont = lucasFont.createDefaultFont(18.0, "Roboto", "Light");
	}
});

// ----------------------------------------------------------------------------

function createColour(alpha, red, green, blue) {
	return alpha << 24 | red << 16 | green << 8 | blue;
}

// ----------------------------------------------------------------------------

function getDistance(pos1, pos2) {
	let dx = pos1[0] - pos2[0];
	let dy = pos1[1] - pos2[1];
	let dz = pos1[2] - pos2[2];
	return Math.sqrt(dx*dx + dy*dy + dz*dz);
}

// ----------------------------------------------------------------------------

function drawNametag(x, y, health, text, ping, alpha, distance, colour, afk, skin) {
	if(nametagFont == null) {
		return false;
	}

	alpha *= 0.75;
	let width = nametagWidth;
	health = Math.max(0.0, Math.min(1.0, health));
    armour = Math.max(0.0, Math.min(1.0, armour));

    // Starts at bottom and works it's way up
    // -------------------------------------------
    // Health Bar

	y -= 5;

	if(health > 0.0) {
		let hx = x-width/2;
		let hy = y-10/2;
		let colourB = toColour(0, 0, 0, Math.floor(255.0*alpha)); // Background colour (black)
		drawing.drawRectangle(null, [hx, hy], [width, 8], colourB, colourB, colourB, colourB);
		let colour = createColour(Math.floor(255.0*alpha), Math.floor(255.0-(health*255.0)), Math.floor(health*255.0), 0); // Health bar colour (varies, depending on health)
		drawing.drawRectangle(null, [hx+2, hy+2], [(width-4)*health, 10-6], colour, colour, colour, colour);
	}

	y -= 20;

    // Nametag
	if(nametagFont != null) {
		let size = nametagFont.measure(text, game.width, 0.0, 0.0, nametagFont.size, false, false);
		let colourT = createColour(Math.floor(255.0*alpha), 255, 255, 255);
		nametagFont.render(text, [x-size[0]/2, y-size[1]/2], game.width, 0.0, 0.0, nametagFont.size, colour, false, false, false, true);
	}

    // Go up another 10 pixels for the next part
    y -= 20;

    // AFK Status
	if(afkStatusFont != null) {
		if(afk) {
			let size = afkStatusFont.measure("PAUSED", game.width, 0.0, 0.0, afkStatusFont.size, false, false);
			afkStatusFont.render("PAUSED", [x-size[0]/2, y-size[1]/2], game.width, 0.0, 0.0, afkStatusFont.size, toColour(255, 0, 0, 255), false, false, false, true);
		}
	}

	// Go up another 50 pixels for the next part
    //y -= 30;

	//if(ping != -1) {
	//	if(pingFont != null) {
	//		let size2 = pingFont.measure(ping, game.width, 0.0, 0.0, pingFont.size, false, false);
	//		let colourT2 = createColour(Math.floor(255.0*alpha), 255, 255, 255);
	//		pingFont.render(ping, [x-size2[0]/2, y-size2[1]/2], game.width, 0.0, 0.0, pingFont.size, colourT2, false, false, false, true);
	//	}
	//}
}

// ----------------------------------------------------------------------------

function updateNametags(element) {
	if(localPlayer != null) {
		let playerPos = localPlayer.position;
		let elementPos = element.position;
		let client = getClientFromPlayer(element);

		elementPos[2] += 0.9;

		let screenPos = getScreenFromWorldPosition(elementPos);
		if (screenPos[2] >= 0.0) {
			let health = element.health/100.0;
			if(health > 1.0) {
				health = 1.0;
			}

			let distance = getDistance(playerPos, elementPos);
			if(distance < nametagDistance) {
				if(element.type == ELEMENT_PLAYER) {
					let colour = COLOUR_WHITE;
					let afk = false;
					if(element.getData("v.afk") > 0) {
						afk = true;
					}
					drawNametag(screenPos[0], screenPos[1], health, element.name, 0, 1.0-distance/nametagDistance, distance, colour, afk, element.skin);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------

function getClientFromPlayer(player) {
	let clients = getClients();
	for(let i in clients) {
		if(clients[i].player == player) {
			return clients[i];
		}
	}
}

// ----------------------------------------------------------------------------

addNetworkHandler("armour", function(client, ped, armour) {
	if(ped != null) {
		ped.armour = armour;
	}
});

// ----------------------------------------------------------------------------

addEventHandler("OnDrawnHUD", function(event) {
	let peds = getElementsByType(ELEMENT_PLAYER);
	for(let i in peds) {
		if(peds[i] != localPlayer) {
			updateNametags(peds[i]);
        }
	}
});