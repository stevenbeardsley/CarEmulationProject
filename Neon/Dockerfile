# Use a newer Ubuntu image that includes GLIBCXX_3.4.32
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    binutils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY EngineSim /app/
CMD ["./EngineSim"]
