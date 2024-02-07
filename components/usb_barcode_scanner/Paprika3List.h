
#pragma once

#include <string>
#include "esp_http_client.h"

namespace esphome {
namespace usb_barcode_scanner{

struct ListItem {
    std::string uid = "";
    std::string name;
    ListItem() {}
    ListItem(std::string name) {
        this->name = name;
    }
    ListItem(std::string aUid, std::string aName) {
        this->uid = aUid;
        this->name = aName;
    }
    bool operator==(const ListItem &that) const {
        return name == that.name;
    }
};

class Paprika3List {
    const std::string paprikaRestURL = "https://www.paprikaapp.com/api/v2/";
public:
    Paprika3List();
    ~Paprika3List();
    void updateListItem(ListItem item);
    void setEmail(std::string email);
    void setPassword(std::string password);
    void setListID(std::string listID);
protected:
    esp_http_client_handle_t initHttpClient();
    void cleanHttpClient(esp_http_client_handle_t client);
    void login();
    optional<std::vector<ListItem>> list();
    void createItem(ListItem item);
    esp_http_client_handle_t client;
    char* receive_buffer;
    std::string email;
    std::string password;
    std::string listID = "8708FA53-B32E-4809-9483-047DC13D4B81-5665-000004B5487830A8";
    std::string paprikaBearerToken = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpYXQiOjE3MDcwNjIzMzgsImVtYWlsIjoicGFwcmlrYUBmbHlpbmctc3RhbXBlLmRlIn0.yvyXfkmJZoF_bSxzJHFvPFIdQ-KTDFuair89tM22Slk"; // TODO: remove
};

}
}
