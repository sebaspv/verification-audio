import warnings

from speechbrain.inference.speaker import SpeakerRecognition
from torch import nn

warnings.filterwarnings("ignore")

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


MODEL_ID = "speechbrain/spkrec-ecapa-voxceleb"
PATH_SPEAKER = "./src/samples/spk1_snt1.wav"
PATH_COMPARE = "./src/samples/spk2_snt1.wav"


if __name__ == "__main__":
    verification = SpeakerRecognition.from_hparams(source=MODEL_ID)
    waveform_speaker = verification.load_audio(PATH_SPEAKER)
    waveform_comparison = verification.load_audio(PATH_COMPARE)
    batch_speaker = waveform_speaker.unsqueeze(0)
    batch_comparison = waveform_comparison.unsqueeze(0)

    score, decision = cos_similarity(verification, batch_speaker, batch_comparison)
    score = score[0]
    decision = decision[0]

    if decision == 1:
        print("Authorized")
    else:
        print("Not authorized")
