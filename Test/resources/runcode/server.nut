// ----------------------------------------------------------------------------

addCommandHandler("sse", function(szCommand, szParams, pClient) {
	if(!pClient.administrator) {
		return false;
	}

    if(!szParams || szParams == "") {
        messageClient("Syntax: /sse <code>", pClient, findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game));
        return false;
    }

	compilestring(szParams)();
	print("Squirrel server code executed: " + szParams);
	messageClient("Squirrel server code executed: " + szParams, pClient, COLOUR_YELLOW);
});

// ----------------------------------------------------------------------------

addCommandHandler("ssr", function(szCommand, szParams, pClient) {
	if(!pClient.administrator) {
		return false;
	}

    if(!szParams || szParams == "") {
        messageClient("Syntax: /sse <code>", pClient, findResourceByName("v-utils").exports.getSyntaxMessageColour(gta.game));
        return false;
    }

	local szOutput = compilestring("return " + szParams)();

	print("Squirrel server code executed: " + szParams);
	print("Returns: " + szOutput);

	messageClient("Squirrel server code executed: " + szParams, pClient, COLOUR_YELLOW);
	messageClient("Returns: " + szOutput, pClient, COLOUR_YELLOW);
});

// ----------------------------------------------------------------------------