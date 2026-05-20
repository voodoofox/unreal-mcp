"""Shared connection holder  - avoids double-importing the main module."""

import functools
import logging

import anyio

logger = logging.getLogger("UnrealMCP")

_get_connection = None

def set_getter(fn):
    global _get_connection
    _get_connection = fn

def get_unreal_connection():
    if _get_connection is None:
        raise RuntimeError("Connection getter not initialized")
    return _get_connection()

def async_tool(mcp):
    """Decorator: registers a sync tool function as async (runs in thread pool).

    Uses anyio.to_thread.run_sync (not asyncio.to_thread) so the blocking
    socket I/O runs in a worker thread that integrates properly with
    FastMCP's anyio event loop  - letting parallel tool calls proceed
    concurrently instead of serialising behind a single thread.
    """
    def decorator(func):
        @functools.wraps(func)
        async def wrapper(*args, **kwargs):
            logger.info(f"async_tool DISPATCH: {func.__name__}")
            result = await anyio.to_thread.run_sync(lambda: func(*args, **kwargs))
            logger.info(f"async_tool COMPLETE: {func.__name__}")
            return result
        return mcp.tool()(wrapper)
    return decorator
