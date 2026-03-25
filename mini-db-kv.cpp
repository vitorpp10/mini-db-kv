#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

struct RecordHeader {
    uint8_t magic;
    int key_len;
    int val_len;
} __attribute__((packed));

class MiniDB {
private:
    int fd;
    std::string filepath;
    std::map<std::string, off_t> index;

    void load_index() {
        lseek(fd, 0, SEEK_SET);

        std::cout << "[INIT] Lendo arquivo para criar indice..." << std::endl;

        while (true) {
            off_t current_pos = lseek(fd, 0, SEEK_CUR);

            RecordHeader header;
            ssize_t bytes_read = read(fd, &header, sizeof(RecordHeader));

            if (bytes_read <= 0) break;

            if (header.magic != 0x42) {
                std::cerr << "Erro: Arquivo corrompido ou formato invalido!" << std::endl;
                break;
            }

            std::vector<char> key_buffer(header.key_len);
            read(fd, key_buffer.data(), header.key_len);
            
            lseek(fd, header.val_len, SEEK_CUR);

            std::string key(key_buffer.begin(), key_buffer.end());
            index[key] = current_pos;
        }
    }

public:
    MiniDB(const std::string& path) : filepath(path) {
        fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        
        if (fd < 0) {
            perror("Erro ao abrir arquivo");
            exit(1);
        }

        load_index();
    }

    ~MiniDB() {
        if (fd >= 0) close(fd);
    }

    void set(const std::string& key, const std::string& value) {
        RecordHeader header;
        header.magic = 0x42;
        header.key_len = key.size();
        header.val_len = value.size();

        off_t position = lseek(fd, 0, SEEK_END);

        write(fd, &header, sizeof(RecordHeader));
        write(fd, key.data(), key.size());
        write(fd, value.data(), value.size());
        
        index[key] = position;
    }

    bool get(const std::string& key, std::string& out_value) {
        if (index.find(key) == index.end()) {
            return false;
        }

        off_t offset = index[key];

        lseek(fd, offset, SEEK_SET);

        RecordHeader header;
        read(fd, &header, sizeof(RecordHeader));

        lseek(fd, header.key_len, SEEK_CUR);

        std::vector<char> buffer(header.val_len);
        read(fd, buffer.data(), header.val_len);

        out_value.assign(buffer.begin(), buffer.end());
        return true;
    }
};

int main() {
    MiniDB db("meubanco.db");

    std::cout << "--- Escrevendo Dados ---" << std::endl;
    db.set("user:1", "Carlos");
    db.set("user:2", "Ana");
    
    db.set("user:1", "Carlos Silva"); 

    std::cout << "--- Lendo Dados ---" << std::endl;
    
    std::string val;
    if (db.get("user:1", val)) {
        std::cout << "Valor de user:1 -> " << val << std::endl;
    }

    if (db.get("user:2", val)) {
        std::cout << "Valor de user:2 -> " << val << std::endl;
    }

    return 0;
}
