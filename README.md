# MCDK-ASSISTANT
为MCMOD开发提供MCP相关的功能和工具，帮助AI更好地理解和使用MCP进行开发。

## MCP客户端配置

支持标准MCP客户端接入，以下配置以 `Roo Code` 为例。

```jsonc
{
    // Roo Code MCP Settings
    "mcpServers": {
		"minecraft_mod_assistant": {
			"url": "http://localhost:18766/sse",
			"name": "Minecraft Mod Assistant MCP Server(MCDK)",
			"alwaysAllow": [
				"search_all",
				"search_api",
				"search_enum",
				"search_event",
				"search_wiki",
				"get_netease_diff",
				"get_netease_jsonui",
				"search_qumod",
				"list_knowledge",
				"read_knowledge",
				"search_netease_guide",
				"search_game_assets",
				"get_jsonui_reference",
				"diagnose_ui",
				"generate_ui_fullstack",
				"draw_pixels_batch",
				"canvas_new",
				"canvas_preview",
				"apply_outline",
				"canvas_save",
				"apply_dithering",
				"apply_palette_quantize",
				"apply_shadow",
				"canvas_info",
				"canvas_load",
				"draw_line",
				"draw_pixel",
				"draw_circle",
				"draw_rect",
				"extract_palette",
				"fill_flood",
				"fill_gradient",
				"fill_rect"
			],
			"timeout": 30
		}
    }
}
```

### VSCode（Copilot）

VSCode 暂不支持直接连接 SSE，需通过 `mcp-remote` 桥接，配置在 [`.vscode/mcp.json`](.vscode/mcp.json)：

```jsonc
{
    "servers": {
        "minecraft_mod_assistant": {
            // 依赖nodejs环境
            "command": "npx",
            "args": [
                "mcp-remote",
                "http://localhost:18766/sse",
                "--transport",
                "sse-only"
            ]
        }
    }
}
```