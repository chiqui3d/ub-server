/**
 *
 * HTTP Status Codes
 *
 * https://github.com/j-ulrich/http-status-codes-cpp
 * 
 * @version 1.5.0
 * @author Jochen Ulrich <jochenulrich@t-online.de>
 * @copyright Licensed under Creative Commons CC0 (http://creativecommons.org/publicdomain/zero/1.0/)
 * 
 * @modified by: José Carlos Campos @chiqui3d
 * 
 * It has been modified only to put the names in the upper case, so that it is a bit clearer.
 * 
 * @copyright Apache 2 http://www.apache.org/licenses/LICENSE-2.0
 * 
 */

#ifndef HTTP_STATUS_CODES_H
#define HTTP_STATUS_CODES_H

/*! Enum for the HTTP status codes.
 */
enum HTTP_STATUS_CODE {
    /*####### 1xx - Informational #######*/

    /** Indicates an interim response for communicating connection status
     * or request progress prior to completing the requested action and
     * sending a final response.
     */
    HTTP_STATUS_CONTINUE = 100,           /*!< Indicates that the initial part of a request has been received and has not yet been rejected by the server. */
    HTTP_STATUS_SWITCHING_PROTOCOLS = 101, /*!< INDICATES that the server understands and is willing to comply with the client's request, via the Upgrade header field, for a change in the application protocol being used on this connection. */
    HTTP_STATUS_PROCESSING = 102,         /*!< Is an interim response used to inform the client that the server has accepted the complete request, but has not yet completed it. */
    HTTP_STATUS_EARLY_HINTS = 103,         /*!< Indicates to the client that the server is likely to send a final response with the header fields included in the informational response. */

    /*####### 2xx - Successful #######*/

    /* Indicates that the client's request was successfully received,
     * understood, and accepted.
     */
    HTTP_STATUS_OK = 200,                          /*!< Indicates that the request has succeeded. */
    HTTP_STATUS_CREATED = 201,                     /*!< Indicates that the request has been fulfilled and has resulted in one or more new resources being created. */
    HTTP_STATUS_ACCEPTED = 202,                    /*!< Indicates that the request has been accepted for processing, but the processing has not been completed. */
    HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION = 203, /*!< Indicates that the request was successful but the enclosed payload has been modified from that of the origin server's 200 (OK) response by a transforming proxy. */
    HTTP_STATUS_NO_CONTENT = 204,                   /*!< Indicates that the server has successfully fulfilled the request and that there is no additional content to send in the response payload body. */
    HTTP_STATUS_RESET_CONTENT = 205,                /*!< Indicates that the server has fulfilled the request and desires that the user agent reset the \"document view\", which caused the request to be sent, to its original state as received from the origin server. */
    HTTP_STATUS_PARTIAL_CONTENT = 206,              /*!< Indicates that the server is successfully fulfilling a range request for the target resource by transferring one or more parts of the selected representation that correspond to the satisfiable ranges found in the requests's Range header field. */
    HTTP_STATUS_MULTISTATUS = 207,                 /*!< Provides status for multiple independent operations. */
    HTTP_STATUS_ALREADY_REPORTED = 208,             /*!< Used inside a DAV:propstat response element to avoid enumerating the internal members of multiple bindings to the same collection repeatedly. [RFC 5842] */
    HTTP_STATUS_IM_USED = 226,                      /*!< The server has fulfilled a GET request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance. */

    /*####### 3xx - Redirection #######*/

    /* Indicates that further action needs to be taken by the user agent
     * in order to fulfill the request.
     */
    HTTP_STATUS_MULTIPLE_CHOICES = 300,   /*!< Indicates that the target resource has more than one representation, each with its own more specific identifier, and information about the alternatives is being provided so that the user (or user agent) can select a preferred representation by redirecting its request to one or more of those identifiers. */
    HTTP_STATUS_MOVED_PERMANENTLY = 301,  /*!< Indicates that the target resource has been assigned a new permanent URI and any future references to this resource ought to use one of the enclosed URIs. */
    HTTP_STATUS_FOUND = 302,             /*!< Indicates that the target resource resides temporarily under a different URI. */
    HTTP_STATUS_SEE_OTHER = 303,          /*!< Indicates that the server is redirecting the user agent to a different resource, as indicated by a URI in the Location header field, that is intended to provide an indirect response to the original request. */
    HTTP_STATUS_NOT_MODIFIED = 304,       /*!< Indicates that a conditional GET request has been received and would have resulted in a 200 (OK) response if it were not for the fact that the condition has evaluated to false. */
    HTTP_STATUS_USE_PROXY = 305,          /*!< \deprecated \parblock Due to security concerns regarding in-band configuration of a proxy. \endparblock
                                             The requested resource MUST be accessed through the proxy given by the Location field. */
    HTTP_STATUS_TEMPORARY_REDIRECT = 307, /*!< Indicates that the target resource resides temporarily under a different URI and the user agent MUST NOT change the request method if it performs an automatic redirection to that URI. */
    HTTP_STATUS_PERMANENT_REDIRECT = 308, /*!< The target resource has been assigned a new permanent URI and any future references to this resource ought to use one of the enclosed URIs. [...] This status code is similar to 301 Moved Permanently (Section 7.3.2 of rfc7231), except that it does not allow rewriting the request method from POST to GET. */

    /*####### 4xx - Client Error #######*/

    /* Indicates that the client seems to have erred.
     */
    HTTP_STATUS_BAD_REQUEST = 400,                  /*!< Indicates that the server cannot or will not process the request because the received syntax is invalid, nonsensical, or exceeds some limitation on what the server is willing to process. */
    HTTP_STATUS_UNAUTHORIZED = 401,                /*!< Indicates that the request has not been applied because it lacks valid authentication credentials for the target resource. */
    HTTP_STATUS_PAYMENT_REQUIRED = 402,             /*!< *Reserved* */
    HTTP_STATUS_FORBIDDEN = 403,                   /*!< Indicates that the server understood the request but refuses to authorize it. */
    HTTP_STATUS_NOT_FOUND = 404,                    /*!< Indicates that the origin server did not find a current representation for the target resource or is not willing to disclose that one exists. */
    HTTP_STATUS_METHOD_NOT_ALLOWED = 405,            /*!< Indicates that the method specified in the request-line is known by the origin server but not supported by the target resource. */
    HTTP_STATUS_NOT_ACCEPTABLE = 406,               /*!< Indicates that the target resource does not have a current representation that would be acceptable to the user agent, according to the proactive negotiation header fields received in the request, and the server is unwilling to supply a default representation. */
    HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED = 407, /*!< Is similar to 401 (Unauthorized), but indicates that the client needs to authenticate itself in order to use a proxy. */
    HTTP_STATUS_REQUEST_TIMEOUT = 408,              /*!< Indicates that the server did not receive a complete request message within the time that it was prepared to wait. */
    HTTP_STATUS_CONFLICT = 409,                    /*!< Indicates that the request could not be completed due to a conflict with the current state of the resource. */
    HTTP_STATUS_GONE = 410,                        /*!< Indicates that access to the target resource is no longer available at the origin server and that this condition is likely to be permanent. */
    HTTP_STATUS_LENGTH_REQUIRED = 411,              /*!< Indicates that the server refuses to accept the request without a defined Content-Length. */
    HTTP_STATUS_PRECONDITION_FAILED = 412,          /*!< Indicates that one or more preconditions given in the request header fields evaluated to false when tested on the server. */
    HTTP_STATUS_CONTENT_TOO_LARGE = 413,             /*!< Indicates that the server is refusing to process a request because the request payload is larger than the server is willing or able to process. */
    HTTP_STATUS_PAYLOAD_TOO_LARGE = 413,             /*!< Alias for HTTP_STATUS_ContentTooLarge for backward compatibility. */
    HTTP_STATUS_URI_TOO_LONG = 414,                  /*!< Indicates that the server is refusing to service the request because the request-target is longer than the server is willing to interpret. */
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE = 415,        /*!< Indicates that the origin server is refusing to service the request because the payload is in a format not supported by the target resource for this method. */
    HTTP_STATUS_RANGE_NOT_SATISFIABLE = 416,         /*!< Indicates that none of the ranges in the request's Range header field overlap the current extent of the selected resource or that the set of ranges requested has been rejected due to invalid ranges or an excessive request of small or overlapping ranges. */
    HTTP_STATUS_EXPECTATION_FAILED = 417,           /*!< Indicates that the expectation given in the request's Expect header field could not be met by at least one of the inbound servers. */
    HTTP_STATUS_IM_A_TEAPOT = 418,                   /*!< Any attempt to brew coffee with a teapot should result in the error code 418 I'm a teapot. */
    HTTP_STATUS_MIS_DIRECTED_REQUEST = 421,          /*!< Indicates that the request was directed at a server that is unable or unwilling to produce an authoritative response for the target URI. */
    HTTP_STATUS_UNPROCESSABLE_CONTENT = 422,        /*!< Means the server understands the content type of the request entity (hence a 415(Unsupported Media Type) status code is inappropriate), and the syntax of the request entity is correct (thus a 400 (Bad Request) status code is inappropriate) but was unable to process the contained instructions. */
    HTTP_STATUS_UNPROCESSABLE_ENTITY = 422,         /*!< Alias for HTTP_STATUS_UnprocessableContent for backward compatibility. */
    HTTP_STATUS_LOCKED = 423,                      /*!< Means the source or destination resource of a method is locked. */
    HTTP_STATUS_FAILED_DEPENDENCY = 424,            /*!< Means that the method could not be performed on the resource because the requested action depended on another action and that action failed. */
    HTTP_STATUS_TOO_EARLY = 425,                    /*!< Indicates that the server is unwilling to risk processing a request that might be replayed. */
    HTTP_STATUS_UPGRADE_REQUIRED = 426,             /*!< Indicates that the server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol. */
    HTTP_STATUS_PRECONDITION_REQUIRED = 428,        /*!< Indicates that the origin server requires the request to be conditional. */
    HTTP_STATUS_TOO_MANY_REQUESTS = 429,             /*!< Indicates that the user has sent too many requests in a given amount of time (\"rate limiting\"). */
    HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431, /*!< Indicates that the server is unwilling to process the request because its header fields are too large. */
    HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS = 451,  /*!< This status code indicates that the server is denying access to the resource in response to a legal demand. */

    /*####### 5xx - Server Error #######*/
    
    /* Indicates that the server is aware that it has erred
     * or is incapable of performing the requested method.
     */
    HTTP_STATUS_INTERNAL_SERVER_ERROR = 500,           /*!< Indicates that the server encountered an unexpected condition that prevented it from fulfilling the request. */
    HTTP_STATUS_NOT_IMPLEMENTED = 501,                /*!< Indicates that the server does not support the functionality required to fulfill the request. */
    HTTP_STATUS_BAD_GATEWAY = 502,                    /*!< Indicates that the server, while acting as a gateway or proxy, received an invalid response from an inbound server it accessed while attempting to fulfill the request. */
    HTTP_STATUS_SERVICE_UNAVAILABLE = 503,            /*!< Indicates that the server is currently unable to handle the request due to a temporary overload or scheduled maintenance, which will likely be alleviated after some delay. */
    HTTP_STATUS_GATEWAY_TIMEOUT = 504,                /*!< Indicates that the server, while acting as a gateway or proxy, did not receive a timely response from an upstream server it needed to access in order to complete the request. */
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED = 505,       /*!< Indicates that the server does not support, or refuses to support, the protocol version that was used in the request message. */
    HTTP_STATUS_VARIANT_ALSO_NEGOTIATES = 506,         /*!< Indicates that the server has an internal configuration error: the chosen variant resource is configured to engage in transparent content negotiation itself, and is therefore not a proper end point in the negotiation process. */
    HTTP_STATUS_INSUFFICIENT_STORAGE = 507,           /*!< Means the method could not be performed on the resource because the server is unable to store the representation needed to successfully complete the request. */
    HTTP_STATUS_LOOP_DETECTED = 508,                  /*!< Indicates that the server terminated an operation because it encountered an infinite loop while processing a request with "Depth: infinity". [RFC 5842] */
    HTTP_STATUS_NOT_EXTENDED = 510,                   /*!< \deprecated \parblock Obsoleted as the experiment has ended and there is no evidence of widespread use. \endparblock
                                                         The policy for accessing the resource has not been met in the request. [RFC 2774] */
    HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED = 511, /*!< Indicates that the client needs to authenticate to gain network access. */

    HTTP_STATUS_XXX_MAX = 1023
};

static inline char HTTP_STATUS_IS_INFORMATIONAL(int code) { return (code >= 100 && code < 200); } /*!< \returns \c true if the given \p code is an informational code. */
static inline char HTTP_STATUS_IS_SUCCESSFUL(int code) { return (code >= 200 && code < 300); }    /*!< \returns \c true if the given \p code is a successful code. */
static inline char HTTP_STATUS_IS_REDIRECTION(int code) { return (code >= 300 && code < 400); }   /*!< \returns \c true if the given \p code is a redirectional code. */
static inline char HTTP_STATUS_IS_CLIENT_ERROR(int code) { return (code >= 400 && code < 500); }   /*!< \returns \c true if the given \p code is a client error code. */
static inline char HTTP_STATUS_IS_SERVER_ERROR(int code) { return (code >= 500 && code < 600); }   /*!< \returns \c true if the given \p code is a server error code. */
static inline char HTTP_STATUS_IS_ERROR(int code) { return (code >= 400); }                       /*!< \returns \c true if the given \p code is any type of error code. */

/*! Returns the standard HTTP reason phrase for a HTTP status code.
 * \param code An HTTP status code.
 * \return The standard HTTP reason phrase for the given \p code or \c NULL if no standard
 * phrase for the given \p code is known.
 */
static inline const char *HTTP_STATUS_REASON(int code) {
    switch (code) {

        /*####### 1xx - Informational #######*/
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 102:
            return "Processing";
        case 103:
            return "Early Hints";

        /*####### 2xx - Successful #######*/
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 207:
            return "Multi-Status";
        case 208:
            return "Already Reported";
        case 226:
            return "IM Used";

        /*####### 3xx - Redirection #######*/
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";

        /*####### 4xx - Client Error #######*/
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Content Too Large";
        case 414:
            return "URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 418:
            return "I'm a teapot";
        case 421:
            return "Misdirected Request";
        case 422:
            return "Unprocessable Content";
        case 423:
            return "Locked";
        case 424:
            return "Failed Dependency";
        case 425:
            return "Too Early";
        case 426:
            return "Upgrade Required";
        case 428:
            return "Precondition Required";
        case 429:
            return "Too Many Requests";
        case 431:
            return "Request Header Fields Too Large";
        case 451:
            return "Unavailable For Legal Reasons";

        /*####### 5xx - Server Error #######*/
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 506:
            return "Variant Also Negotiates";
        case 507:
            return "Insufficient Storage";
        case 508:
            return "Loop Detected";
        case 510:
            return "Not Extended";
        case 511:
            return "Network Authentication Required";

        default:
            return 0;
    }
}

#endif /* HTTP_STATUS_CODES_H */
