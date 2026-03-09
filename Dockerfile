FROM ubuntu:22.04

# Install build tools (g++)
RUN apt-get update && apt-get install -y g++

# Set working directory inside container
WORKDIR /app

# Copy all project files into the image
COPY url.shortner.cpp .
COPY crow_all.h .
COPY include/ include/
COPY index.html .

# Compile the C++ API for Linux (WS2_32 is not needed on Linux, just pthread)
RUN g++ url.shortner.cpp -I include -o url.shortner -pthread -O3

# Run the compiled binary
CMD ["./url.shortner"]
