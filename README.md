# YouTube Transcript Fetcher in C

A lightweight, dependency-free (besides `libcurl`) command-line tool written in C to download the transcript of any YouTube video. It directly calls YouTube's internal API, mimicking an iOS client, to fetch the transcript data without requiring an API key.

## Features

-   **No API Key Needed**: Fetches transcripts by mimicking a legitimate client request.
-   **Language Selection**: Specify the desired language for the transcript (e.g., "en", "es", "fr").
-   **Lightweight**: Written in plain C with `libcurl` as the only external dependency.
-   **Simple Output**: Prints the transcript text directly to standard output for easy piping and redirection.
-   **Self-Contained**: The required `cJSON` library is included in the source.

## Dependencies

To build this project, you will need:

-   A C compiler (like `gcc` or `clang`)
-   `make`
-   `libcurl` (development version)

You can install `libcurl` on Debian/Ubuntu with:

```bash
sudo apt-get update
sudo apt-get install libcurl4-openssl-dev
```

On openSUSE:

```bash
sudo zypper install libcurl-devel
```

On macOS (using Homebrew):

```bash
brew install curl
```

## Building

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Zibri/youtube_transcript
    cd youtube_transcript
    ```

2.  **Compile the code:**
    Simply run `make` to use the provided `Makefile`.

    ```bash
    make
    ```

    This will compile the source files (`youtube_transcript.c`, `cJSON.c`) and create an executable named `youtube_transcript`. The executable is stripped of debug symbols to reduce its size.

3.  **Clean up build files:**
    To remove the compiled object files and the executable, run:
    ```bash
    make clean
    ```

## Usage

Run the program from your terminal with the YouTube video ID as the first argument. You can optionally provide a language code as the second argument (defaults to "en").

**Syntax:**

```
./youtube_transcript <video_id> [language_code]
```

### Examples:

1.  **Get a transcript in English (default):**
    ```bash
    ./youtube_transcript dQw4w9WgXcQ
    ```

2.  **Get a transcript in French:**
    ```bash
    ./youtube_transcript dQw4w9WgXcQ fr
    ```

3.  **Save the transcript to a file:**
    ```bash
    ./youtube_transcript dQw4w9WgXcQ > transcript.txt
    ```

## How It Works

This program works by reverse-engineering the API call made by the YouTube iOS application to fetch video transcripts.

1.  **Protobuf Simulation**: The tool manually constructs a binary protobuf (Protocol Buffers) message. This message contains the video ID and the desired language parameters, formatted exactly as the official app would send them.
2.  **Encoding**: The protobuf data is Base64 encoded and then URL-encoded to ensure it can be safely transmitted within a JSON payload.
3.  **JSON Payload**: A JSON object is created, which includes a `context` block to identify the client as a specific version of the YouTube iOS app. The encoded protobuf data is included as the `params` value.
4.  **API Request**: A POST request is sent to YouTube's internal API endpoint (`/youtubei/v1/get_transcript`) using `libcurl`. The request includes specific `User-Agent` and `X-Youtube-Client` headers to appear as a legitimate request from an iPhone.
5.  **Response Parsing**: The JSON response from the API is parsed using the included `cJSON` library to navigate the complex object structure and extract the transcript segments.
6.  **Output**: Each transcript segment is printed to the standard output.

## Disclaimer

This tool uses an unofficial, internal YouTube API. This API can change at any time without warning, which may cause this tool to stop working. This project is intended for educational purposes to demonstrate how to interact with web APIs in C. Use at your own risk.