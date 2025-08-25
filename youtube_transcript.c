#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <curl/curl.h>
#include "cJSON.h"

// --- Client Configuration ---
// These values mimic a specific YouTube iOS client.
#define CLIENT_NAME "IOS"
#define CLIENT_VERSION "19.45.4"
#define USER_AGENT "com.google.ios.youtube/19.45.4 (iPhone16,2; U; CPU iOS 18_1_0 like Mac OS X;)"
#define DEVICE_MODEL "iPhone16,2"

// Struct to hold the response from curl in memory
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function to write curl response to our memory struct
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        fprintf(stderr, "Error: Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Base64 encode function
char *base64_encode(const unsigned char *data, size_t input_length) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t i = 0; i < (3 - (input_length % 3)) % 3; i++) {
        encoded_data[output_length - 1 - i] = '=';
    }

    encoded_data[output_length] = '\0';
    return encoded_data;
}

// URL encode function using libcurl for robust encoding
char *url_encode(const char *str) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    char *encoded = curl_easy_escape(curl, str, 0);
    if (!encoded) {
        curl_easy_cleanup(curl);
        return NULL;
    }
    char *result = malloc(strlen(encoded) + 1);
    if (result) {
        strcpy(result, encoded);
    }

    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <video_id> [language_code]\n", argv[0]);
        fprintf(stderr, "Example: %s dQw4w9WgXcQ en\n", argv[0]);
        return 1;
    }

    const char *video_id = argv[1];
    const char *lang_code = (argc > 2) ? argv[2] : "en";

    // --- Step 1: Manually construct the language protobuf bytes ---
    size_t lang_code_len = strlen(lang_code);
    size_t lang_proto_len = 1 + 1 + 3 + 1 + 1 + lang_code_len + 1 + 1;
    unsigned char *lang_proto_bytes = malloc(lang_proto_len);
    if (!lang_proto_bytes) {
        fprintf(stderr, "Memory allocation failed for lang_proto_bytes\n");
        return 1;
    }

    int offset = 0;
    lang_proto_bytes[offset++] = 0x0A;
    lang_proto_bytes[offset++] = 0x03;
    memcpy(lang_proto_bytes + offset, "asr", 3); offset += 3;
    lang_proto_bytes[offset++] = 0x12;
    lang_proto_bytes[offset++] = (unsigned char)lang_code_len;
    memcpy(lang_proto_bytes + offset, lang_code, lang_code_len); offset += lang_code_len;
    lang_proto_bytes[offset++] = 0x1A;
    lang_proto_bytes[offset++] = 0x00;

    char *encoded_lang_b64 = base64_encode(lang_proto_bytes, lang_proto_len);
    char *url_encoded_lang = url_encode(encoded_lang_b64);
    free(lang_proto_bytes);

    // --- Step 2: Manually construct the params protobuf bytes ---
    size_t video_id_len = strlen(video_id);
    size_t url_encoded_lang_len = strlen(url_encoded_lang);
    size_t params_proto_len = 1 + 1 + video_id_len + 1 + 1 + url_encoded_lang_len + 1 + 1;
    unsigned char *params_proto_bytes = malloc(params_proto_len);
    if (!params_proto_bytes) {
        fprintf(stderr, "Memory allocation failed for params_proto_bytes\n");
        free(encoded_lang_b64);
        free(url_encoded_lang);
        return 1;
    }

    offset = 0;
    params_proto_bytes[offset++] = 0x0A;
    params_proto_bytes[offset++] = (unsigned char)video_id_len;
    memcpy(params_proto_bytes + offset, video_id, video_id_len); offset += video_id_len;
    params_proto_bytes[offset++] = 0x12;
    params_proto_bytes[offset++] = (unsigned char)url_encoded_lang_len;
    memcpy(params_proto_bytes + offset, url_encoded_lang, url_encoded_lang_len); offset += url_encoded_lang_len;
    params_proto_bytes[offset++] = 0x18;
    params_proto_bytes[offset++] = 0x01;

    char *params_b64 = base64_encode(params_proto_bytes, params_proto_len);
    free(params_proto_bytes);

    // --- Step 3: Construct the full JSON payload ---
    cJSON *root = cJSON_CreateObject();
    cJSON *context = cJSON_CreateObject();
    cJSON *client = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "context", context);
    cJSON_AddItemToObject(context, "client", client);
    cJSON_AddStringToObject(client, "hl", "en");
    cJSON_AddStringToObject(client, "gl", "US");
    cJSON_AddStringToObject(client, "clientName", CLIENT_NAME);
    cJSON_AddStringToObject(client, "clientVersion", CLIENT_VERSION);
    cJSON_AddStringToObject(client, "deviceModel", DEVICE_MODEL);
    cJSON_AddStringToObject(client, "userAgent", USER_AGENT);
    cJSON_AddStringToObject(client, "timeZone", "UTC");
    cJSON_AddStringToObject(root, "params", params_b64);

    char *payload = cJSON_PrintUnformatted(root);

    // --- Step 4: Execute the curl command ---
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl) {
        char url[]="https://www.youtube.com/youtubei/v1/get_transcript";
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char user_agent_header[512];
        snprintf(user_agent_header, sizeof(user_agent_header), "User-Agent: %s", USER_AGENT);
        headers = curl_slist_append(headers, user_agent_header);
        headers = curl_slist_append(headers, "X-Youtube-Client-Name: 3");
        char client_version_header[256];
        snprintf(client_version_header, sizeof(client_version_header), "X-Youtube-Client-Version: %s", CLIENT_VERSION);
        headers = curl_slist_append(headers, client_version_header);
        headers = curl_slist_append(headers, "Origin: https://youtube.com");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json_response = cJSON_Parse(chunk.memory);
            if (!json_response) {
                fprintf(stderr, "Error: Failed to parse JSON response.\n");
            } else {
                // Manually traverse the JSON object.
                cJSON *actions = cJSON_GetObjectItem(json_response, "actions");
                if (actions && cJSON_IsArray(actions)) {
                    cJSON *action = cJSON_GetArrayItem(actions, 0);
                    cJSON *elementsCommand = cJSON_GetObjectItem(action, "elementsCommand");
                    cJSON *transformEntityCommand = cJSON_GetObjectItem(elementsCommand, "transformEntityCommand");
                    cJSON *arguments = cJSON_GetObjectItem(transformEntityCommand, "arguments");
                    cJSON *transformTranscriptSegmentListArguments = cJSON_GetObjectItem(arguments, "transformTranscriptSegmentListArguments");
                    cJSON *overwrite = cJSON_GetObjectItem(transformTranscriptSegmentListArguments, "overwrite");
                    cJSON *initialSegments = cJSON_GetObjectItem(overwrite, "initialSegments");

                    if (initialSegments && cJSON_IsArray(initialSegments)) {
                        cJSON *segment;
                        cJSON_ArrayForEach(segment, initialSegments) {
                            cJSON *transcriptSegmentRenderer = cJSON_GetObjectItem(segment, "transcriptSegmentRenderer");
                            cJSON *snippet = cJSON_GetObjectItem(transcriptSegmentRenderer, "snippet");
                            cJSON *elementsAttributedString = cJSON_GetObjectItem(snippet, "elementsAttributedString");
                            cJSON *content = cJSON_GetObjectItem(elementsAttributedString, "content");
                            if (cJSON_IsString(content) && (content->valuestring != NULL)) {
                                printf("%s\n", content->valuestring);
                            }
                        }
                    } else {
                         fprintf(stderr, "Could not find transcript in API response. The video may not have a transcript for the selected language.\n");
                    }
                } else {
                     fprintf(stderr, "Could not find 'actions' array in API response.\n");
                }
                cJSON_Delete(json_response);
            }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    free(chunk.memory);
    free(payload);
    cJSON_Delete(root);
    free(encoded_lang_b64);
    free(url_encoded_lang);
    free(params_b64);

    return 0;
}
