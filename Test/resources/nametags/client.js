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
    [255, 255, 255],            		// GTA3
	[255, 255, 255],            		// GTAVC
	[255, 255, 255],            		// GTASA
	[255, 255, 255],            		// GTAUG
	[255, 255, 255],            		// GTAIV
	[255, 255, 255],            		// GTAEFLC
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// INVALID
	[255, 255, 255],            		// Mafia
	[255, 255, 255],            		// Mafia II
	[255, 255, 255],            		// Mafia III
	[255, 255, 255],            		// Mafia DE
];

// ----------------------------------------------------------------------------

addEventHandler("OnResourceReady", function(event, resource) {
	if (resource == thisResource) {
		nametagFont = lucasFont.createDefaultFont(12.0, "Roboto", "Light");
		afkStatusFont = lucasFont.createDefaultFont(18.0, "Roboto", "Light");
	}
});

// ----------------------------------------------------------------------------

function drawNametag(x, y, health, text, ping, alpha, distance, colour, afk) {
	console.warn(`START DRAWING NAMETAG FOR ${text}`);

	if(nametagFont == null) {
		return false;
	}

	console.warn(`NAMETAG FONT VALID`);

	alpha *= 0.75;
	let width = nametagWidth;
	health = Math.max(0.0, Math.min(1.0, health));

    // Starts at bottom and works it's way up
    // -------------------------------------------
    // Health Bar

	y -= 5;

	console.warn(`HEALTH DRAWING`);

	if(health > 0.0) {
		let hx = x-width/2;
		let hy = y-10/2;
		let colour = toColour(Math.floor(255.0*alpha), Math.floor(255.0-(health*255.0)), Math.floor(health*255.0), 0); // Health bar colour (varies, depending on health)
		drawing.drawRectangle(null, [hx+2, hy+2], [(width-4)*health, 10-6], colour, colour, colour, colour);
	}

	console.warn(`HEALTH DRAWN`);

	y -= 20;

	console.warn(`NAME DRAWING`);

    // Nametag
	if(nametagFont != null) {
		console.warn(`MEASURING SIZE`);
		let size = nametagFont.measure(text, 200, 0.5, 0.0, nametagFont.size, false, false);
		console.warn(`RENDERING TEXT`);
		nametagFont.render(text, new Vec2(x-size.x/2, y-size.y/2), 200, 0.5, 0.0, nametagFont.size, COLOUR_WHITE, false, false, false, false);
	}

	console.warn(`NAME DRAWN`);

    // Go up another 10 pixels for the next part
    y -= 20;

    // AFK Status
	//if(afkStatusFont != null) {
	//	if(afk) {
	//		let size = afkStatusFont.measure("PAUSED", game.width, 0.0, 0.0, afkStatusFont.size, false, false);
	//		afkStatusFont.render("PAUSED", [x-size[0]/2, y-size[1]/2], game.width, 0.0, 0.0, afkStatusFont.size, toColour(255, 0, 0, 255), false, false, false, true);
	//	}
	//}

	// Go up another 50 pixels for the next part
    //y -= 30;

	//if(ping != -1) {
	//	if(pingFont != null) {
	//		let size2 = pingFont.measure(ping, game.width, 0.0, 0.0, pingFont.size, false, false);
	//		let colourT2 = toColour(Math.floor(255.0*alpha), 255, 255, 255);
	//		pingFont.render(ping, [x-size2[0]/2, y-size2[1]/2], game.width, 0.0, 0.0, pingFont.size, colourT2, false, false, false, true);
	//	}
	//}
}

// ----------------------------------------------------------------------------

function updateNametag(element) {
	if(localPlayer != null) {
		let playerPos = localPlayer.position;
		let elementPos = element.position;

		elementPos[1] += 0.9;

		let screenPos = getScreenFromWorldPosition(elementPos);
		if (screenPos[2] >= 0.0) {
			let health = 100.0;
			//let health = element.health/100.0;
			//if(health > 1.0) {
			//	health = 1.0;
			//}

			let distance = playerPos.distance(elementPos);
			if(distance < nametagDistance) {
				if(element.type == ELEMENT_PLAYER) {
					let colour = COLOUR_WHITE;
					let afk = false;
					//if(element.getData("v.afk") > 0) {
					//	afk = true;
					//}
					console.warn(`DRAWING NAMETAG FOR ${element.id}`);
					drawNametag(screenPos.x, screenPos.y, health, element.name, 0, 1.0-distance/nametagDistance, distance, colour, afk);
				}
			}
		}
	}
}

// ----------------------------------------------------------------------------

addEventHandler("OnDrawnHUD", function(event) {
	console.log("RENDER");
	let players = getElementsByType(ELEMENT_PLAYER);
	for(let i in players) {
		if(players[i] != localPlayer) {
			updateNametag(players[i]);
        }
	}
});

// ----------------------------------------------------------------------------