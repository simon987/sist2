import os.path
from typing import List

from pydantic import BaseModel

from sist2 import WebOptions


class Sist2Frontend(BaseModel):
    name: str
    jobs: List[str]
    web_options: WebOptions
    running: bool = False

    auto_start: bool = False
    enable_monitoring: bool = True
    extra_query_args: str = ""
    custom_url: str = None

    def get_log_path(self, log_folder: str):
        return os.path.join(log_folder, f"frontend-{self.name}.log")

    @staticmethod
    def create_default(name: str):
        return Sist2Frontend(
            name=name,
            web_options=WebOptions(),
            jobs=[]
        )
