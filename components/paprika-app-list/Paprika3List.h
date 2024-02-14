
#pragma once

#include <string>
#include "esp_http_client.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace paprika_app_list{

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

class Paprika3List : public Component {
    const std::string paprikaRestURL = "https://www.paprikaapp.com/api/v2/";
public:
    Paprika3List();
    ~Paprika3List();
    void updateListItem(const std::string& name);
    void setEmail(const std::string& email);
    void setPassword(const std::string& password);
    void setListID(const std::string& listID);
    void dump_config() override;
    float get_setup_priority() const override;
protected:
    esp_http_client_handle_t initHttpClient();
    void cleanHttpClient(esp_http_client_handle_t client);
    void login();
    optional<std::vector<ListItem>> list();
    void createItem(const ListItem& item);
    char* receive_buffer;
    std::string email;
    std::string password;
    std::string listID;
    std::string paprikaBearerToken = "";
    
};

template<typename... Ts> 
class Paprika3ListAddAction : public Action<Ts...> {
public:
    explicit Paprika3ListAddAction(Paprika3List *parent) : parent_(parent) {}
    
    void set_format(const std::string &fmt) { this->format_ = fmt; }

    void play(Ts... x) override {
        this->parent_->updateListItem(format(x...));
    }

    std::string format(std::string s) {
        char data[1024];
        snprintf(data, sizeof(data), this->format_.c_str(), s.c_str());
        return std::string(data);
    }

protected:
    Paprika3List *parent_;
    std::string format_;
};

}
}
