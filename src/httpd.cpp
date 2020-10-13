#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"
#include <dl_lib_matrix3d.h>
#include <file_utils.h>
#include <ArduinoJson.h>

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static int8_t detection_enabled = 0;
static int8_t recognition_enabled = 0;
static int8_t is_enrolling = 0;

/* Scratch buffer size */
#define SCRATCH_BUFSIZE 8192

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN)

struct file_server_data
{
    /* Base path of file storage */
    char base_path[256];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

typedef struct
{
    size_t size;  //number of values used for filtering
    size_t index; //current value index
    size_t count; //value count
    int sum;
    int *values; //array to be filled with values
} ra_filter_t;

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID)
    {
        return httpd_resp_send(req, (const char *)index_ov3660_html_gz, index_ov3660_html_gz_len);
    }
    return httpd_resp_send(req, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    static char json_response[1024];

    sensor_t *s = esp_camera_sensor_get();
    char *p = json_response;
    *p++ = '{';

    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
    p += sprintf(p, "\"face_detect\":%u,", detection_enabled);
    p += sprintf(p, "\"face_enroll\":%u,", is_enrolling);
    p += sprintf(p, "\"face_recognize\":%u", recognition_enabled);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t *s = esp_camera_sensor_get();
    int res = 0;

    if (!strcmp(variable, "framesize"))
    {
        if (s->pixformat == PIXFORMAT_JPEG)
            res = s->set_framesize(s, (framesize_t)val);
    }
    else if (!strcmp(variable, "quality"))
        res = s->set_quality(s, val);
    else if (!strcmp(variable, "contrast"))
        res = s->set_contrast(s, val);
    else if (!strcmp(variable, "brightness"))
        res = s->set_brightness(s, val);
    else if (!strcmp(variable, "saturation"))
        res = s->set_saturation(s, val);
    else if (!strcmp(variable, "gainceiling"))
        res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (!strcmp(variable, "colorbar"))
        res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "awb"))
        res = s->set_whitebal(s, val);
    else if (!strcmp(variable, "agc"))
        res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "aec"))
        res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror"))
        res = s->set_hmirror(s, val);
    else if (!strcmp(variable, "vflip"))
        res = s->set_vflip(s, val);
    else if (!strcmp(variable, "awb_gain"))
        res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "agc_gain"))
        res = s->set_agc_gain(s, val);
    else if (!strcmp(variable, "aec_value"))
        res = s->set_aec_value(s, val);
    else if (!strcmp(variable, "aec2"))
        res = s->set_aec2(s, val);
    else if (!strcmp(variable, "dcw"))
        res = s->set_dcw(s, val);
    else if (!strcmp(variable, "bpc"))
        res = s->set_bpc(s, val);
    else if (!strcmp(variable, "wpc"))
        res = s->set_wpc(s, val);
    else if (!strcmp(variable, "raw_gma"))
        res = s->set_raw_gma(s, val);
    else if (!strcmp(variable, "lenc"))
        res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect"))
        res = s->set_special_effect(s, val);
    else if (!strcmp(variable, "wb_mode"))
        res = s->set_wb_mode(s, val);
    else if (!strcmp(variable, "ae_level"))
        res = s->set_ae_level(s, val);
    else if (!strcmp(variable, "face_detect"))
    {
        detection_enabled = val;
        if (!detection_enabled)
        {
            recognition_enabled = 0;
        }
    }
    else if (!strcmp(variable, "face_enroll"))
        is_enrolling = val;
    else if (!strcmp(variable, "face_recognize"))
    {
        recognition_enabled = val;
        if (recognition_enabled)
        {
            detection_enabled = val;
        }
    }
    else
    {
        res = -1;
    }

    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t *out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if (!detection_enabled || fb->width > 400)
    {
        size_t fb_len = 0;
        if (fb->format == PIXFORMAT_JPEG)
        {
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        }
        else
        {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
        return res;
    }

    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if (!s)
    {
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);

    if (!s)
    {
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    Serial.printf("FACE: %uB %ums %s%d\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start) / 1000), detected ? "DETECTED " : "", face_id);
    return res;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[64];

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while (true)
    {

        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            if (!detection_enabled || fb->width > 400)
            {
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            break;
        }
    }

    last_frame = 0;
    return res;
}

String listDirectories(File dir)
{
    String response = "";
    Serial.println("rewind");
    dir.rewindDirectory();

    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            Serial.println("**nomorefiles**");
            break;
        }

        // Recurse for directories, otherwise print the file size
        if (entry.isDirectory())
        {
            response += String("<a href=/open?path=") + String(entry.name()) + String(">") + String(entry.name()) + String("</a>") + String("</br>");
        }
        else
        {
            response += String(entry.name()) + String("</br>");
        }
        Serial.println("antes close");
        entry.close();
        Serial.println("depois close");
    }
    Serial.println("devolvendo string" + response);
    return String("<h1>Hello Secure World!</h1> <br> List files:</br>" + response);
}

static esp_err_t list_handler(httpd_req_t *req)
{
    //listDir(SD_MMC, "/" , 0);
    File f = SD_MMC.open("/", "r");
    httpd_resp_set_type(req, "text/html");
    Serial.println("#######");
    String list = listDirectories(f);
    Serial.println("#######");
    Serial.println("retorno" + list);
    const char *dados = list.c_str();

    Serial.println("tamanho:" + strlen(dados));

    httpd_resp_send(req, dados, strlen(dados));

    return ESP_OK;
}

/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
static const char *get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

esp_err_t get_handler(httpd_req_t *req)
{

    Serial.println("get handler");
    size_t filename_len = httpd_req_get_url_query_len(req);
    Serial.println("" + String(filename_len));

    if (filename_len == 0)
    {
        char *resp_str = "Please specify a filename. eg. file?somefile.txt";
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }

    char variable[128] = {
        0,
    };
    char *buf;
    size_t buf_len;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            Serial.println("***********");
            Serial.println("path");
            if (httpd_query_key_value(buf, "path", variable, sizeof(variable)) == ESP_OK)
            {
                Serial.println(variable);
            }

            Serial.println(buf);
            Serial.println("***********");

            //listDir(SD_MMC, "/" , 0
            File f = SD_MMC.open(variable, "r");
            Serial.println(f);

            httpd_resp_set_type(req, "text/html");
            String list = listDirectories(f);
            const char *dados = list.c_str();

            Serial.println("tamanho:" + strlen(dados));

            free(buf);
            httpd_resp_send(req, dados, strlen(dados));
        }
        free(buf);
    }
    return ESP_OK;
}

static char *retrivePathfromRequest(httpd_req_t *req)
{

    char *buf;
    size_t buf_len;
    buf_len = httpd_req_get_url_query_len(req) + 1;

    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            return buf;
        }
        free(buf);
    }
    return buf;
}

static esp_err_t lista_handler(httpd_req_t *req)
{

    DynamicJsonDocument root(1024);
    JsonArray data = root.createNestedArray("path");
    String path = retrivePathfromRequest(req);
    path = path.substring(5, path.length());
    Serial.println(path);

    File f = SD_MMC.open(path, "r");
    f.rewindDirectory();

    while (true)
    {
        Serial.println(".");
        File entry = f.openNextFile();
        if (!entry)
        {
            Serial.println("**nomorefiles**");
            break;
        }

        JsonObject obj = data.createNestedObject();
        obj["path"] = String(entry.name());
        obj["directory"] = String(entry.isDirectory());
        entry.close();
    }

    char dados[1000];
    serializeJson(root, dados);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, dados, strlen(dados));
    return ESP_OK;
}

static esp_err_t image_handler(httpd_req_t *req)
{

    String path = retrivePathfromRequest(req);
    path = path.substring(5, path.length());
    Serial.println(path);

    File f = SD_MMC.open(path, "r");

    if (f)
    {
        Serial.println("file found");
        int filesize = f.size();
        Serial.println("size:" + String(filesize));
        httpd_resp_set_type(req, "image/jpeg");
        String header = "inline; filename=" + path;
        httpd_resp_set_hdr(req, "Content-Disposition", header.c_str());
        httpd_resp_set_hdr(req, "Content-Length:", (const char *)filesize);
        httpd_resp_set_type(req, "image/jpeg");
        Serial.println("file read - start");

        char buf[1024] = {'\0'};
        int siz = f.size();
        Serial.println("size:" + String(siz));

        uint16_t i = 0;
        while (f.available())
        {
            buf[i] = f.read();
            Serial.println("file read.");
            httpd_resp_set_type(req, "image/jpeg");
            httpd_resp_send(req, (const char *)buf, sizeof(buf));
            i++;
        }

        f.close();
    }

    Serial.println("file read - end");
    httpd_resp_send_chunk(req, NULL, 0);
    f.close();
    return ESP_OK;
}

/* HTTP GET handler for downloading files */
static esp_err_t download_get_handler(httpd_req_t *req)
{
    File fd;

    String path = retrivePathfromRequest(req);
    path = path.substring(5, path.length());
    Serial.println(path);

    fd = SD_MMC.open(path.c_str(), "r");
    if (!fd)
    {
        Serial.println("Fail");
        return ESP_FAIL;
    }

    Serial.println(".");
    httpd_resp_set_type(req, "image/jpeg");

    Serial.println(".");
    

    // this is going to get the number of bytes in the file and give us the value in an integer
    int fileSize = fd.size();
    int chunkSize=1024;
    //This is a character array to store a chunk of the file.
    //We'll store 1024 characters at a time
    char buf[chunkSize];
    int numberOfChunks=(fileSize/chunkSize)+1;
    
    int remainingChunks=fileSize;
    for (int i=1; i <= numberOfChunks; i++){
      if (remainingChunks-chunkSize < 0){
        chunkSize=remainingChunks;
      }
      fd.read((uint8_t *)buf, chunkSize);
      remainingChunks=remainingChunks-chunkSize;
      httpd_resp_send_chunk(req, (const char *)buf, sizeof(buf));
    }


    /* Close file after sending complete */
    fd.close();

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t file_download = {
        .uri = "/xupa", // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = download_get_handler,
        .user_ctx = NULL // Pass server data as context
    };

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL};

    httpd_uri_t cmd_uri = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL};

    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL};

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    httpd_uri_t list_dir_uri = {
        .uri = "/list",
        .method = HTTP_GET,
        .handler = list_handler,
        .user_ctx = NULL};

    httpd_uri_t file_download_uri = {
        .uri = "/open", // Match all URIs of type /path/to/file
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL // Pass server data as context
    };

    httpd_uri_t file_lista_uri = {
        .uri = "/lista",
        .method = HTTP_GET,
        .handler = lista_handler,
        .user_ctx = NULL // Pass server data as context
    };

    httpd_uri_t file_image_uri = {
        .uri = "/img",
        .method = HTTP_GET,
        .handler = image_handler,
        .user_ctx = NULL // Pass server data as context
    };

    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &file_download);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
        httpd_register_uri_handler(camera_httpd, &list_dir_uri);
        httpd_register_uri_handler(camera_httpd, &file_download_uri);
        httpd_register_uri_handler(camera_httpd, &file_lista_uri);
        httpd_register_uri_handler(camera_httpd, &file_image_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}
