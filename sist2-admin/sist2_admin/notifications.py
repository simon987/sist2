import asyncio
from typing import List


class Notifications:
    def __init__(self):
        self._subscribers: List[Subscribe] = []

    def subscribe(self, ob):
        self._subscribers.append(ob)

    def unsubscribe(self, ob):
        self._subscribers.remove(ob)

    def notify(self, notification: dict):
        for ob in self._subscribers:
            ob.notify(notification)


class Subscribe:
    def __init__(self, notifications: Notifications):
        self._queue = []
        self._notifications = notifications

    async def __aenter__(self):
        self._notifications.subscribe(self)
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        self._notifications.unsubscribe(self)

    def notify(self, notification: dict):
        self._queue.append(notification)

    async def notifications(self):
        while True:
            try:
                yield self._queue.pop(0)
            except IndexError:
                await asyncio.sleep(0.1)
