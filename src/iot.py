import warnings
import wave

import paho.mqtt.client as mqtt
from speechbrain.inference.speaker import SpeakerRecognition
from torch import nn

warnings.filterwarnings("ignore")

MODEL_ID = "speechbrain/spkrec-ecapa-voxceleb"
PATH_SPEAKER = "./src/samples/voz_sebastian.wav"
PATH_COMPARE = "./recorded_audio.wav"

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


# Configuración MQTT
mqtt_broker = "broker.emqx.io"
mqtt_port = 1883
mqtt_topic_audio = "audio/verification"
mqtt_topic_response = "audio/response"
mqtt_username = "emqx"
mqtt_password = "public"

# Conexión y configuración del cliente MQTT
client = mqtt.Client()
client.username_pw_set(mqtt_username, mqtt_password)

# Callback cuando se recibe un mensaje en el tópico de audio
def on_message(client, userdata, msg):
    print("Mensaje recibido en el tópico:", msg.topic)
    
    # El mensaje recibido es el audio en formato binario
    audio_data = msg.payload
    print(len(audio_data))
    # Guardar el audio en un archivo WAV
    with wave.open(PATH_COMPARE, "wb") as wf:
        wf.setnchannels(1)  # Mono
        wf.setsampwidth(2)  # 16-bit por muestra
        wf.setframerate(32000)  # 32kHz de frecuencia de muestreo
        wf.writeframes(audio_data)
    
    # Cargar el modelo de reconocimiento de hablante
    verification = SpeakerRecognition.from_hparams(source=MODEL_ID)
    waveform_speaker = verification.load_audio(PATH_SPEAKER)
    waveform_comparison = verification.load_audio(PATH_COMPARE)
    
    # Realizar la verificación del hablante
    batch_speaker = waveform_speaker.unsqueeze(0)
    batch_comparison = waveform_comparison.unsqueeze(0)

    score, decision = cos_similarity(verification, batch_speaker, batch_comparison)
    score = score[0]
    decision = decision[0]
    
    if decision:
        print("Autorizado")
        client.publish(mqtt_topic_response, payload="authorized", qos=1)
    else:
        print("No autorizado")
        client.publish(mqtt_topic_response, payload="not authorized", qos=1)


# Conectar a la red MQTT
def connect_mqtt():
    print("Conectando a MQTT Broker...")
    client.connect(mqtt_broker, mqtt_port)
    client.subscribe(mqtt_topic_audio, qos=1)
    print(f"Conectado a {mqtt_broker} y suscrito al tópico {mqtt_topic_audio}")


# Iniciar el ciclo de espera para los mensajes MQTT
def loop_mqtt():
    client.loop_forever()


if __name__ == "__main__":
    # Configurar el callback de los mensajes recibidos
    client.on_message = on_message
    
    # Conectar y empezar a escuchar los mensajes
    connect_mqtt()
    loop_mqtt()
