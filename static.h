#pragma once

#define SITE_NAME "/c/chan"
#define TIME_LENGTH 40

// Static files
// (state of the art shitty html4 that is)
#define RESPONSE_HEADER \
"HTTP/1.1 200 OK\n" \
"Content-Type: text/html\n" \
"Content-Length: "

#define CSS_FILE \
"body {" \
    "font-family: Arial, sans-serif;" \
    "background-color: #ffffff;" \
"}" \
/* Header */ \
"#header {" \
    "text-align:center;" \
"}" \
"#header table {" \
    "border: 0 none;" \
    "margin: 0 auto;" \
    "width: 380px;" \
"}" \
/* Form table */ \
"tr td:first-child {" \
    "vertical-align: top;" \
    "text-align:right;" \
"}" \
/* Post */ \
".subject {" \
    "color: #DB0A5B;" \
    "font-weight: bold;" \
"}" \
".name {" \
    "color: #16A085;" \
    "font-weight: bold;" \
"}" \
".id {" \
    "color: #666666;" \
"}" \
".reply {" \
    "margin-left:10px;" \
"}"

#define HEADER_FILE \
"<html>" \
"<head>" \
    "<title>" SITE_NAME "</title>" \
    "<meta charset=utf-8>" \
    "<style>" \
    CSS_FILE \
    "</style>" \
"</head>" \
"<body>"

#define FOOTER_FILE \
    "<hr>" \
"</body>" \
"</html>"

// subject, name, time (utc), time, id, comment
#define POST_ELEMENT \
    "<div class='post'>" \
        "<p class='tagline'>" \
            "<span class='subject'>%s</span> " \
            "<span class='name'>%s</span> " \
            "<time datetime='%s'>%s</time> " \
            "<a href='/post/%i' class='id'>#%i</a>" \
        "</p>" \
        "<p class='comment'>%s</p>" \
    "</div>"

#define POST_REPLY_ELEMENT \
    "<div class='post reply'>" \
        "<p class='tagline'>" \
            "<span class='subject'>%s</span> " \
            "<span class='name'>%s</span> " \
            "<time datetime='%s'>%s</time> " \
            "<a href='/post/%i' class='id'>#%i</a>" \
        "</p>" \
        "<p class='comment'>%s</p>" \
    "</div>"

#define FORM_TEMPLATE(prepend) \
    "<form action=/post method=post>" \
        "<table>" \
        prepend \
        "<tr>" \
            "<td valign=top align=right><label for=name>Name:</label></td>" \
            "<td><input name='name'></td>" \
        "</tr>" \
        "<tr>" \
            "<td valign=top align=right><label for=subject>Subject:</label></td>" \
            "<td><input name='subject'></td>" \
        "</tr>" \
        "<tr>" \
            "<td valign=top align=right><label for=comment>Comment:</label></td>" \
            "<td><textarea required name=comment cols=30 rows=12></textarea></td>" \
        "</tr>" \
        "<tr>" \
            "<td></td>" \
            "<td><input type=submit value=Post!></td>" \
        "</tr>" \
        "</table>" \
    "</form>" \

#define INDEX_FILE_HEADER \
    HEADER_FILE \
    "<div id='header'>" \
    "<h1><a href=/>" SITE_NAME "</a></h1>" \
    FORM_TEMPLATE() \
    "</div>"

#define REPLY_FILE_HEADER \
    HEADER_FILE \
    "<div id='header'>" \
    "<h1><a href=/>" SITE_NAME "</a></h1>" \
    FORM_TEMPLATE(\
        "<tr>" \
            "<td colspan=2 style='text-align:center !important;'>" \
                "Replying to post..." \
                "<input type=text readonly value='%s' name=reply_to style='display:none;'>" /* shitty html hack */ \
            "</td>" \
        "</tr>") \
    "</div>"
