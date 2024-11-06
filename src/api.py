import wave
from fastapi.middleware.cors import CORSMiddleware

import uvicorn
from fastapi import FastAPI, Request
import warnings

from speechbrain.inference.speaker import SpeakerRecognition
from torch import nn

warnings.filterwarnings("ignore")

MODEL_ID = "speechbrain/spkrec-ecapa-voxceleb"
PATH_SPEAKER = "./src/samples/spk1_snt1.wav"
PATH_COMPARE = "./muchomejor.wav"

def cos_similarity(
    model: SpeakerRecognition,
    wavs1,
    wavs2,
    wav1_lens=None,
    wav2_lens=None,
    threshold=0.25,
):
    """Compute cosine similarity of embeddings"""
    similarity = nn.CosineSimilarity(dim=-1, eps=1e-6)
    emb1 = model.encode_batch(wavs1, wav1_lens, normalize=False)
    emb2 = model.encode_batch(wavs2, wav2_lens, normalize=False)
    score = similarity(emb1, emb2)
    return score, score > threshold



app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.post("/api/upload_audio")
async def upload_audio(request: Request):
    sample_rate = int(request.headers.get("Sample-Rate", 8000))  # Default to 32000 Hz
    channels = int(request.headers.get("Channels", 1))  # Default to mono
    bits_per_sample = int(request.headers.get("Bits-Per-Sample", 16)) 

    raw_audio_data = await request.body()

    if bits_per_sample != 16:
        return {"error": "Only 16-bit audio is supported."}

    wav_file_path = "recorded_audio.wav"
    with wave.open(wav_file_path, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(bits_per_sample // 8)  # 16-bit is 2 bytes
        wf.setframerate(sample_rate)
        wf.writeframes(raw_audio_data)

    verification = SpeakerRecognition.from_hparams(source=MODEL_ID)
    score, decision = verification.verify_files(PATH_SPEAKER, PATH_COMPARE)
    print(score)
    if decision == 1:
        return {"message": "authorized"}
    else:
        return {"message": "not authorized"}


if __name__ == "__main__":
    uvicorn.run(app, host="localhost", port=8000)
