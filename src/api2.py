import ssl
import sys
import warnings
import wave

import paho.mqtt.client
import uvicorn
from fastapi import FastAPI, Request, Response, status
from fastapi.middleware.cors import CORSMiddleware
import paho.mqtt.publish as publish
import paho.mqtt.subscribe as subscribe


from speechbrain.inference.speaker import SpeakerRecognition
from torch import nn

warnings.filterwarnings("ignore")

MODEL_ID = "speechbrain/spkrec-ecapa-voxceleb"
audios = ["./src/samples/audio_tono.wav", "./src/samples/recorded_audio.wav", "./src/samples/audio_albert.wav", "./src/samples/audio_luis.wav"]
PATH_COMPARE = "./recorded_audio.wav"
mqtt_broker = "127.0.0.1"
mqtt_port = 8001
mqtt_topic_audio = "audio/verification"

def cos_similarity(
    model: SpeakerRecognition,
    wavs1,
    wavs2,
    wav1_lens=None,
    wav2_lens=None,
    threshold=0.55,
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
async def upload_audio(request: Request, response: Response):
    sample_rate = int(request.headers.get("Sample-Rate", 32000))  # Default to 4000 Hz
    channels = int(request.headers.get("Channels", 1))  # Default to mono
    bits_per_sample = int(request.headers.get("Bits-Per-Sample", 16))

    raw_audio_data = await request.body()
    wav_file_path = "recorded_audio.wav"
    with wave.open(wav_file_path, "wb") as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(bits_per_sample // 8)  # 16-bit is 2 bytes
        wf.setframerate(sample_rate)
        wf.writeframes(raw_audio_data)
    
    flag = False
    verification = SpeakerRecognition.from_hparams(source=MODEL_ID)
    for PATH_SPEAKER in audios:
        waveform_speaker = verification.load_audio(PATH_SPEAKER)
        waveform_comparison = verification.load_audio(PATH_COMPARE)
        batch_speaker = waveform_speaker.unsqueeze(0)
        batch_comparison = waveform_comparison.unsqueeze(0)
        score, decision = cos_similarity(verification, batch_speaker, batch_comparison)
        score = score[0]
        decision = decision[0]
        if decision[0]:
            flag = True
    
    if flag:
        publish.single(mqtt_topic_audio, "1", hostname=mqtt_broker, port=mqtt_port)
        return {"message": "authorized"}
    else:
        response.status_code = 401
        return {"message": "not authorized"}


if __name__ == "__main__":
    uvicorn.run(app, host="localhost", port=8000)
