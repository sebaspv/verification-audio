from typing import List

import numpy as np
import torchaudio
import uvicorn
from fastapi import FastAPI
from torch import nn

app = FastAPI()

@app.get("/authorize")
async def authorize(recording: List[int]) -> int:

    pass

if __name__ == "__main__":
    uvicorn.run(app)