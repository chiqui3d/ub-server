#ifndef MIME_TYPES_H
#define MIME_TYPES_H

// Currently using magic library: libmagic
// https://www.oryx-embedded.com/doc/mime_8c_source.html
// https://github.com/kobbyowen/MegaMimes/blob/master/src/MegaMimes.c
// https://wiki.debian.org/MIME/etc/mime.types

typedef struct
{
    const char *extension;
    const char *type;
} MimeType;


// https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types
static const MimeType mimeTypeList[] =
    {

        // Text MIME types
        {".css", "text/css"},
        {".csv", "text/csv"},
        {".htc", "text/x-component"},
        {".htm", "text/html"},
        {".html", "text/html"},
        {".shtm", "text/html"},
        {".shtml", "text/html"},
        {".stm", "text/html"},
        {".txt", "text/plain"},
        {".vcf", "text/vcard"},
        {".vcard", "text/vcard"},
        {".xml", "text/xml"},

        // Image MIME types
        {".gif", "image/gif"},
        {".ico", "image/vnd.microsoft"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".svg", "image/svg+xml"},
        {".tif", "image/tiff"},
        {".tiff", "image/tiff"},
        {".webp", "image/webp"},
        {".bmp", "image/bmp"},

        // Audio MIME types
        {".aac", "audio/x-aac"},
        {".aif", "audio/x-aiff"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/x-wav"},
        {".wma", "audio/x-ms-wma"},
        {".weba", "audio/webm"},

        // Video MIME types
        {".avi", "video/x-msvideo"},
        {".flv", "video/x-flv"},
        {".mov", "video/quicktime"},
        {".mp4", "video/mp4"},
        {".mpg", "video/mpeg"},
        {".mpeg", "video/mpeg"},
        {".wmv", "video/x-ms-wmv"},
        {".webm", "video/webm"},
        {".webp", "video/webp"},

        // font types
        {".eot", "application/vnd.ms-fontobject"},
        {".ttf", "font/ttf"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".otf", "font/otf"},

        // Application MIME types
        {".bz", "application/x-bzip"},
        {".bz2", "application/x-bzip2"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".gz", "application/gzip"},
        {".gzip", "application/gzip"},
        {".js", "text/javascript"},
        {".mjs", "text/javascript"},
        {".json", "application/json"},
        {".jsonld", "application/ld+json"},
        {".ogg", "application/ogg"},
        {".pdf", "application/pdf"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".rar", "application/x-rar-compressed"},
        {".rtf", "application/rtf"},
        {".7z", "application/x-7z-compressed"},
        {".tar", "application/x-tar"},
        {".tgz", "application/x-gzip"},
        {".xht", "application/xhtml+xml"},
        {".xhtml", "application/xhtml+xml"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".zip", "application/zip"}
    };

#endif // MIME_TYPES_H