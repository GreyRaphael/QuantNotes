# LLM

## open-webui

```bash
# start server in LM studio(Vulkan better than AMD driver)
lms server start
# rev proxy
./wstunnel client -R tcp://[::]:1234:localhost:1234 -P YourPassword wss://remote_host:443 --no-color true
```

```bash
# in remote_host use webui
# 1. check models
curl -L http://remote_host:1234/v1/models  -H "Authorization: Bearer YourApiKey"
# 2. docker run webui
docker run -d -p 3000:8080 \
  -e OPENAI_API_BASE_URL="http://host.containers.internal:1234/v1" \
  -e OPENAI_API_KEY="YourApiKey" \
  -e RAG_EMBEDDING_ENGINE="" \
  -v open-webui:/app/backend/data \
  --name open-webui \
  --add-host=host.containers.internal:host-gateway \
  ghcr.io/open-webui/open-webui:main

# check status
docker logs open-webui
curl -I http://127.0.0.1:3000
# remove service
docker rm -f open-webui
```

## llama-server + open-webui

`./llama-server --models-preset models.ini`
> for AMD, download [Windows x64 (Vulkan)](https://github.com/ggml-org/llama.cpp/releases)

[llama-server parameters](https://manpages.debian.org/unstable/llama.cpp-tools/llama-server.1.en.html)

```ini
[*]
ngl = 99
flash-attn = on
sleep-idle-seconds = 300
reasoning = off
cache-type-k = q8_0
cache-type-v = q8_0
port = 1234
api-key = YOUR_API_KEY

[gemma4-e4b-Q5_K_M]
model = F:/LMStudio/models/HauhauCS/Gemma-4-E4B-Uncensored-HauhauCS-Aggressive/Gemma-4-E4B-Uncensored-HauhauCS-Aggressive-Q5_K_M.gguf
mmproj = F:/LMStudio/models/HauhauCS/Gemma-4-E4B-Uncensored-HauhauCS-Aggressive/mmproj-Gemma-4-E4B-Uncensored-HauhauCS-Aggressive-f16.gguf

[qwen3.5-4b-Q6_K]
model = F:/LMStudio/models/HauhauCS/Qwen3.5-4B-Uncensored-HauhauCS-Aggressive/Qwen3.5-4B-Uncensored-HauhauCS-Aggressive-Q6_K.gguf
mmproj = F:/LMStudio/models/HauhauCS/Qwen3.5-4B-Uncensored-HauhauCS-Aggressive/mmproj-Qwen3.5-4B-Uncensored-HauhauCS-Aggressive-BF16.gguf

[qwen3.5-9b-Q4_K_M]
model = F:/LMStudio/models/HauhauCS/Qwen3.5-9B-Uncensored-HauhauCS-Aggressive/Qwen3.5-9B-Uncensored-HauhauCS-Aggressive-Q4_K_M.gguf
mmproj = F:/LMStudio/models/HauhauCS/Qwen3.5-9B-Uncensored-HauhauCS-Aggressive/mmproj-Qwen3.5-9B-Uncensored-HauhauCS-Aggressive-BF16.gguf
```

然后使用open-webui