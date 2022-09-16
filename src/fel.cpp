#include <QDebug>

#include "fel.h"
#include "x.h"

fel::fel() = default;

void fel::fel_open_usb(){
    usb_handler.open_usb();
}

void fel::fel_close_usb() {
    usb_handler.close_usb();
}

void fel::fel_scan_chip() {
    usb_handler.fel_init();
    fel_chip_id();
}

void fel::fel_chip_id() {
    send_fel_request(0x001, 0, 0);
    usb_handler.usb_read(&version, sizeof(version));
    read_fel_status();
    version.id = le32_to_cpu(version.id);
    version.firmware = le32_to_cpu(version.firmware);
    version.protocol = le16_to_cpu(version.protocol);
    version.scratchpad = le32_to_cpu(version.scratchpad);
}

void fel::send_fel_request(int type, uint32_t addr, uint32_t length) {
    struct fel_request_t req = {
            .request = cpu_to_le32(type),
            .address = cpu_to_le32(addr),
            .length = cpu_to_le32(length)
    };
    usb_handler.usb_write(&req, sizeof(struct fel_request_t));
}

void fel::read_fel_status() {
    char buf[8];
    usb_handler.usb_read(buf, sizeof(buf));
    qDebug() << buf;
}

chip_version_t fel::fel_get_chip_id() const {
    return version;
}
