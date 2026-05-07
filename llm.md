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