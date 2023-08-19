import os
import logging
import sys
from logging import StreamHandler
from logging.handlers import RotatingFileHandler

MAX_LOG_SIZE = 1 * 1024 * 1024

SIST2_BINARY = os.environ.get("SIST2_BINARY", "/root/sist2")
DATA_FOLDER = os.environ.get("DATA_FOLDER", "/sist2-admin/")
LOG_FOLDER = os.path.join(DATA_FOLDER, "logs")
SCRIPT_FOLDER = os.path.join(DATA_FOLDER, "scripts")
WEBSERVER_PORT = 8080

os.makedirs(LOG_FOLDER, exist_ok=True)
os.makedirs(SCRIPT_FOLDER, exist_ok=True)
os.makedirs(DATA_FOLDER, exist_ok=True)

logger = logging.Logger("sist2-admin")

_log_file = os.path.join(LOG_FOLDER, "sist2-admin.log")
_log_fmt = "%(asctime)s [%(levelname)s] %(message)s"
_log_formatter = logging.Formatter(_log_fmt, datefmt='%Y-%m-%d %H:%M:%S')

console_handler = StreamHandler(sys.stdout)
console_handler.setFormatter(_log_formatter)

file_handler = RotatingFileHandler(_log_file, mode="a", maxBytes=MAX_LOG_SIZE, backupCount=1)
file_handler.setFormatter(_log_formatter)

logger.addHandler(console_handler)
logger.addHandler(file_handler)
