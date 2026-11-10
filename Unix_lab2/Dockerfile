FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends util-linux
COPY work.sh /work.sh
CMD ["sh", "/work.sh"]
