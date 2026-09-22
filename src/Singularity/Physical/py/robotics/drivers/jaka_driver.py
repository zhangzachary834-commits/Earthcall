import asyncio
import json
import logging
from .base_driver import BaseDriver

logger = logging.getLogger(__name__)

class JakaDriver(BaseDriver):
    """
    A concrete driver for a JAKA robotic arm.
    Connects to the robot's TCP socket, parses telemetry (e.g. joint angles),
    and translates it into Earthcall property writes.
    """
    def __init__(self, connection_id, engine_sync, base_slug="robot-base"):
        super().__init__(connection_id, engine_sync)
        self.address = None
        self.reader = None
        self.writer = None
        self.running = False
        self.base_slug = base_slug
        # Assumed relations structure from base: robot-link1, robot-link2, etc.
        self.link_slugs = [f"robot-link{i+1}" for i in range(6)]

    async def connect(self, address, port=10004):
        self.address = (address, port)
        try:
            self.reader, self.writer = await asyncio.open_connection(address, port)
            self.running = True
            logger.info(f"[JakaDriver] Connected to JAKA at {address}:{port}")
            # Start background reading task
            asyncio.create_task(self._read_loop())
        except Exception as e:
            logger.error(f"[JakaDriver] Failed to connect to {address}:{port}: {e}")
            self.running = False

    def disconnect(self):
        self.running = False
        if self.writer:
            self.writer.close()
        logger.info(f"[JakaDriver] Disconnected from {self.address}")

    async def _read_loop(self):
        while self.running and self.reader:
            try:
                # JAKA typically sends JSON or binary. Let's assume JSON for this implementation
                data = await self.reader.readline()
                if not data:
                    break
                self.handle_incoming_data(data.decode('utf-8').strip())
            except Exception as e:
                logger.error(f"[JakaDriver] Error reading data: {e}")
                break
        self.disconnect()

    def handle_incoming_data(self, data):
        """
        Parses incoming JAKA telemetry and maps it to Earthcall PropertyWrites.
        Expected JAKA JSON format mock: {"joint_angles": [0.1, 0.2, 0.3, 0.4, 0.5, 0.6]}
        """
        try:
            payload = json.loads(data)
            if "joint_angles" in payload:
                angles = payload["joint_angles"]
                # For each joint angle, we map it to the corresponding link's rotation property
                # In Earthcall, this drives the actual visual representation via relations
                for i, angle in enumerate(angles):
                    if i < len(self.link_slugs):
                        target_slug = self.link_slugs[i]
                        # Send property write: e.g. update `robot-link1.joint_angle`
                        # A Law inside Earthcall will listen to this property and update the physical channel
                        self.engine_sync.send_property_write(target_slug, "joint_angle", angle)
        except json.JSONDecodeError:
            pass # Ignore invalid JSON
        except Exception as e:
            logger.error(f"[JakaDriver] Error handling data: {e}")

    def send_command(self, command_data):
        """
        Sends a command back to the physical JAKA robot.
        """
        if self.running and self.writer:
            try:
                payload = json.dumps(command_data) + "\n"
                self.writer.write(payload.encode('utf-8'))
            except Exception as e:
                logger.error(f"[JakaDriver] Error sending command: {e}")
