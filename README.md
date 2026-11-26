# VATPHIL EuroScope plugin 1.4.0

Based on [DiscordEuroscope](https://github.com/Kirollos/DiscordEuroscope) by Kirollos Nashaat. EuroScope lists it as **VATPHIL**; the file is `VATPHIL.dll`, installed in `RPHI/Plugins/VATPHIL/`. Commands work as `.vatphil …` or `.discord …`.

Discord Rich Presence for EuroScope, plus optional forwarding of ATC chat to a Discord channel webhook.

## Quick start

1. Put `VATPHIL.dll` and `VATPHIL_config.json` in the same folder — for VATPHIL that is `RPHI/Plugins/VATPHIL/` in your sector package.
2. The VATPHIL build already has the webhook URL baked into the DLL; there is no URL setting in the config.
3. Start EuroScope, then type `.discord webhooktest` to check the webhook.

The plugin stays quiet on startup. It only shows a message when something is wrong (bad config, or Wine without a Discord bridge).

## VATPHIL portal check

As soon as EuroScope loads the plugin, before you connect, it checks your sector file (see below). If you don't have the current AIRAC's `.sct`, a black screen headed *You can't connect to VATSIM* covers EuroScope, so you can't reach Connect. Run the VATPHIL Updater, then press **Try again**. The check runs again every 30 minutes while you're not connected, and each time you disconnect.

When you connect to a VATPHIL position (an `RP…` or `MNL_…` callsign) on the live network, the plugin asks the VATPHIL portal whether you may control it (`GET https://portal.vatphil.com/api/plugin/check?callsign=…&sct=…`). If you may not, a black screen covers the whole monitor EuroScope is on. It has no close button, ignores Alt+F4, and lists every reason that applies:

- You are currently Inactive, you are not permitted to control.
- You have an outdated sector file, please update.
- You currently are not endorsed for this position.
- You are not permitted to control this position.

It has two buttons. **Try again** checks again after you've fixed the problem. **Close EuroScope** closes EuroScope (which disconnects you), since EuroScope is covered; if EuroScope is still connected 20 seconds later, for example because you cancelled its save prompt, the screen comes back. The screen also goes away if you disconnect some other way.

- The portal uses the same rules as its *Can I control?* page (Roster activity, rating, solo validations, endorsements, the S1/S2 approved-aerodrome hours).
- Who you are comes from the VATSIM data feed, which lags about a minute behind a connect. The plugin asks again every 20 seconds until the portal can see you, for up to about 5 minutes.
- **Sector file:** the plugin sends the names of the `.sct` files in your sector package (the Updater names them after the AIRAC, e.g. `2612.sct`). You need the `.sct` for the current AIRAC cycle, or a newer one; an old `.sct` left next to it doesn't matter. Staff can set a different cycle in the portal (**Settings → EuroScope plugin → AIRAC cycle the sector file must be**) for the days before a new cycle's files are out. If the plugin can't find any `.sct`, this part isn't checked.
- Sweatbox and playback connections, observers, ATIS and `_SUP` callsigns are not checked. Supervisors and administrators are only checked for the sector file.
- If the portal can't be reached, nobody is blocked. Staff can turn the check off in the portal's Settings.

## Configuration

`VATPHIL_config.json` lives next to `VATPHIL.dll`. After editing it, type `.discord hotreload` in EuroScope — no restart is needed. If the file is missing, the plugin generates it.

> The webhook URL is not in the config. It is compiled into the DLL (see below). Keep a baked DLL private.

### Webhook (ATC chat → Discord)

The webhook URL is **baked into the DLL**, not read from the config. The VATPHIL build installed in `RPHI/Plugins/VATPHIL/` is already baked.

The public `vatphil-latest` release DLL is a template containing a placeholder, because publishing a baked DLL would publish the webhook URL. Bake the downloaded DLL before using it:

```
WEBHOOK_URL='https://discord.com/api/webhooks/...' \
    python3 tools/bake_webhook.py --dll VATPHIL.dll --out VATPHIL_baked.dll
```

Then install `VATPHIL_baked.dll` as `VATPHIL.dll`. The URL is passed at bake time and is not stored in this repository.

Only the line format is configurable:

```json
"webhook": {
    "line_format": "**{sender}:** {message}"
}
```

- `line_format` — what each chat line looks like in Discord. Missing or empty uses `**{sender}:** {message}`.
- Placeholders: `{sender}` (callsign), `{message}` (chat text), `{time}` (UTC `HH:MM:SS`). Placeholders are replaced once; braces inside a chat message stay literal.
- Only the **ATC** chat is forwarded. Frequency chat, private messages, broadcasts and server messages are never posted.
- Messages are posted under your callsign as the webhook name, and several messages that arrive close together are combined into one post.
- Mentions are disabled, so `@everyone` and role/user mentions in chat never ping anyone.
- Messages are sent from a background thread; EuroScope never waits on Discord.

Anyone who has a baked DLL can extract the URL from it, so do not publish or share a baked DLL. The public release intentionally stays a placeholder. If the URL leaks, delete the Discord webhook, create a new one, and bake the DLL again.

If several controllers run the plugin with the same webhook, every instance will post its copy of the chat. Use the webhook on one relay instance only, or use separate webhooks per controller.

`.discord webhooktest` sends one test line through the configured `line_format` and reports whether Discord accepted it.

### Rich presence entries and images

All display text and image keys are configurable:

| Field | Meaning |
| --- | --- |
| `discord_appid` | Discord application ID (Rich Presence app). |
| `discord_presence_large_image_key` | Default large image asset key. |
| `discord_presence_small_image_key` | Default small image asset key. |
| `sweatbox_bypass` | Skip presence updates while in sweatbox. |
| `states.idle`, `playback`, `sweatbox`, `direct` | Per-state presence. |
| `states.direct.if_observer`, `if_controller`, `if_supervisor` | Overrides depending on your rating/role. |

Each state accepts `state`, `details`, `presence_large_image_key`, `presence_small_image_key`, `presence_large_image_text`, `presence_small_image_text` and an optional `buttons` array (up to 2 `{ "label": "...", "url": "..." }` entries). An empty per-state image key falls back to the global default. Available text placeholders: `{callsign}`, `{rcallsign}`, `{frequency}`, `{current_tracked}`, `{total_inrange}`, `{total_tracked}`.

Image keys refer to **art assets on the Discord application**, not local file paths. To change the actual picture:

1. Open the [Discord Developer Portal](https://discord.com/developers/applications), select the VATPHIL application, then **Rich Presence → Art Assets**.
2. Upload the image and give it a name, for example `vatphil_banner` or `vatphil_logo`.
3. Use that name in `discord_presence_large_image_key` / `discord_presence_small_image_key` (or a state override).
4. Type `.discord hotreload`; restart Discord if it keeps showing the old image.

The source pictures are in `discord-assets/`. The plugin cannot upload artwork to Discord by itself — that step always happens in the Developer Portal.

### Radio callsigns

`radio_callsigns` maps position callsigns to the name shown in the presence details (`{rcallsign}`). The old `VATPHIL_RadioCallsigns.txt` file is no longer used.

```json
"radio_callsigns": {
    "config": {
        "load_from_ese": false,
        "path_to_ese": "..\\..\\..\\"
    },
    "custom_callsigns": {
        "^MNL_CTR$": "Manila Control",
        "^MNL_(\\d+)(?:_)CTR$": "Manila Control ACC $0"
    }
}
```

Set `load_from_ese` to `true` to read position names directly from the `.ese` file(s) in `path_to_ese`; otherwise the regex rules in `custom_callsigns` are used. `.discord hotreload` reports how many callsigns were parsed.

## Building and updating

The `Build VATPHIL plugin` workflow builds the 32-bit DLL on GitHub Actions. It runs automatically on pushes to the `VATPHIL` branch, or manually from the Actions tab. The finished `VATPHIL.dll` is attached to the `vatphil-latest` prerelease, but it is a **template** with a webhook placeholder.

To install a webhook-enabled update, download the DLL and bake it first:

```
WEBHOOK_URL='https://discord.com/api/webhooks/...' \
    python3 tools/bake_webhook.py --dll VATPHIL.dll --out VATPHIL_baked.dll
```

Close EuroScope, replace `VATPHIL.dll` in `RPHI/Plugins/VATPHIL/` with the baked DLL, keep the config, and start EuroScope again. A DLL swap is not covered by `.discord hotreload` — only config changes are.

## Using the plugin on Mac or Linux (Wine)

EuroScope runs under Wine, but Discord runs as a normal Mac or Linux app. Wine can't connect the two on its own, so you need a small free bridge program called [rpc-bridge](https://github.com/EnderIce2/rpc-bridge). You only have to set it up once per Wine prefix (or CrossOver bottle).

When the plugin loads under Wine, it checks for the bridge. If it can't find it, it shows `... Discord can't be reached from Wine on its own ...` in EuroScope, which means the bridge isn't running yet. Follow the steps below. If everything is fine it stays quiet; type `.discord hotreload` if you want to see `Running under Wine ... Discord bridge found.`

The webhook does **not** need the bridge — it talks to Discord over HTTPS directly. Only Rich Presence needs it.

### Mac

1. Download the latest release from the [rpc-bridge releases page](https://github.com/EnderIce2/rpc-bridge/releases) and unzip it.
2. Open Terminal in the unzipped folder and run:
   ```
   chmod +x launchd.sh
   ./launchd.sh install
   ```
3. Run `bridge.exe` with the same Wine prefix / CrossOver bottle you use for EuroScope, and click **Install**.
4. Open the Discord app, start EuroScope, and connect. If EuroScope was already open, type `.discord hotreload`.

### Linux

1. Download the latest release from the [rpc-bridge releases page](https://github.com/EnderIce2/rpc-bridge/releases) and unzip it.
2. Run `bridge.exe` with the same Wine prefix you use for EuroScope (for Lutris: *Run EXE inside Wine prefix*), and click **Install**.
3. Open the Discord app, start EuroScope, and connect. If EuroScope was already open, type `.discord hotreload`.

If you use the Flatpak version of Discord, it also needs permission to share its connection. Run this once:

```
flatpak override --user --filesystem=xdg-run/discord-ipc-0 --filesystem=xdg-run/.flatpak/com.discordapp.Discord:create com.discordapp.Discord
```

To uninstall, run `bridge.exe` again and click **Remove** (on Mac, also run `./launchd.sh remove`).

## Troubleshooting

- `.discord webhooktest` says **No webhook URL is baked into this build** — you are running the template DLL; bake it with `tools/bake_webhook.py` and replace the DLL (then restart EuroScope).
- `.discord webhooktest` reports **404** or **401/403** — the webhook was deleted or the URL is wrong; create/copy it again.
- `.discord webhooktest` reports **network or URL error** — check the internet connection (and any proxy) for Wine; the webhook does not use Discord IPC.
- The webhook works but presence is blank — the Discord application ID or image asset key is wrong, or the Discord bridge isn't running under Wine.
- Config edits do nothing — run `.discord hotreload`; check that you edited the JSON next to the DLL that EuroScope actually loaded.
- `.discord hotreload` shows `Config error: ...` — the named field is missing or has the wrong type; fix it and reload again.
