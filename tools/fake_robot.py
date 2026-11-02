"""A stand-in for the robot, for working on the driving page without the hardware.

Serves web/ in the same way as the ESP32 and answers the page's WebSocket like the
firmware: one page in control, busy for the others, the 0.5 s safety stop, a silent
page losing control after 6 s, the deadzone and speed limit, telemetry ten times
a second. No motors are involved.

    ~/.platformio/penv/bin/python tools/fake_robot.py [port] [--hotspot]

Then open http://localhost:8765. GET /debug/log lists the messages received
and the last telemetry sent (newest last).
"""

import asyncio
import json
import math
import sys
import time
from pathlib import Path

import uvicorn
from starlette.applications import Starlette
from starlette.responses import FileResponse, JSONResponse, Response
from starlette.routing import Route, WebSocketRoute
from starlette.websockets import WebSocket, WebSocketDisconnect

WEB = Path(__file__).resolve().parent.parent / "web"
DEADZONE = 0.08
TURN_DUTY = 0.5
DEADMAN_S = 0.5
SILENT_DROP_S = 6.0
TELEMETRY_S = 0.1
HOTSPOT = "--hotspot" in sys.argv

state = {"client": None, "joy": False, "x": 0.0, "y": 0.0, "r": 0.0, "spin": 0, "limit": 50,
         "heard": 0.0, "drive_heard": 0.0}
log = []


def note(entry):
    log.append({"t": round(time.monotonic(), 3), **entry})
    del log[:-400]


def clear_input():
    state.update(x=0.0, y=0.0, r=0.0, spin=0)


def telemetry():
    if state["joy"] and time.monotonic() - state["drive_heard"] > DEADMAN_S:
        clear_input()
    m = min(math.hypot(state["x"], state["y"]), 1.0) if state["joy"] else 0.0
    share = (m - DEADZONE) / (1 - DEADZONE) if m > DEADZONE else 0.0
    angle = math.degrees(math.atan2(-state["x"], state["y"])) % 360 if share > 0 else 0  # 90 = left
    turn = max(-1.0, min(1.0, state["spin"] + state["r"])) if state["joy"] else 0.0
    return {
        "type": "telemetry",
        "joy_enabled": state["joy"],
        "spin": state["spin"],
        "angle": round(angle) % 360,
        "speed": round(share * state["limit"]),
        "turn": round(turn * TURN_DUTY * state["limit"]),
        "limit": state["limit"],
        "net": "ap" if HOTSPOT else "sta",
        "rssi": None if HOTSPOT else -58,
    }


def handle(msg):
    """Mirrors phoneMessage() in phone_control.cpp. Returns a reply or None."""
    state["heard"] = time.monotonic()
    kind = msg.get("type")
    num = lambda key: msg.get(key) if isinstance(msg.get(key), (int, float)) and math.isfinite(msg.get(key)) else None
    if kind == "ping":
        return {"type": "pong"}
    if kind == "drive" and state["joy"] and None not in (num("x"), num("y"), num("r"), num("limit")) \
            and num("spin") in (1, 0, -1):
        x, y = num("x"), num("y")
        m = math.hypot(x, y)
        if m > 1:
            x, y = x / m, y / m
        state.update(x=x, y=y, r=max(-1.0, min(1.0, num("r"))), spin=int(num("spin")),
                     limit=round(max(10, min(100, num("limit")))), drive_heard=time.monotonic())
    elif kind == "enable_joy":
        state["joy"] = True
        clear_input()
    elif kind == "disable_joy":
        state["joy"] = False
        clear_input()
    elif kind == "estop":
        clear_input()
    return None


async def ws_endpoint(ws: WebSocket):
    await ws.accept()
    if state["client"] is not None:
        await ws.send_text(json.dumps({"type": "busy", "message": "Another device is driving the robot"}))
        await asyncio.sleep(0.3)
        await ws.close(code=1008)
        note({"event": "busy"})
        return
    state.update(client=ws, joy=False, limit=50, heard=time.monotonic())
    clear_input()
    note({"event": "connect"})

    async def send_telemetry():
        while True:
            if time.monotonic() - state["heard"] > SILENT_DROP_S:
                note({"event": "silent page dropped"})
                await ws.close()
                return
            frame = telemetry()
            note({"sent": frame})
            await ws.send_text(json.dumps(frame))
            await asyncio.sleep(TELEMETRY_S)

    sender = asyncio.create_task(send_telemetry())
    try:
        while True:
            text = await ws.receive_text()
            try:
                msg = json.loads(text)
            except ValueError:
                continue
            note({"got": msg})
            reply = handle(msg)
            if reply:
                await ws.send_text(json.dumps(reply))
    except WebSocketDisconnect:
        pass
    finally:
        sender.cancel()
        state.update(client=None, joy=False)
        clear_input()
        note({"event": "disconnect"})


def page(_request):
    return FileResponse(WEB / "index.html", headers={"Cache-Control": "no-cache"})


def static(request):
    path = (WEB / request.path_params["path"]).resolve()
    if WEB not in path.parents or not path.is_file():
        return Response("Not found", status_code=404)
    return FileResponse(path, headers={"Cache-Control": "no-cache"})


def debug_log(_request):
    return JSONResponse(log)


app = Starlette(
    routes=[
        Route("/", page),
        Route("/static/{path:path}", static),
        Route("/debug/log", debug_log),
        WebSocketRoute("/ws", ws_endpoint),
    ]
)

if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    port = int(args[0]) if args else 8765
    uvicorn.run(app, host="127.0.0.1", port=port, log_level="warning")
