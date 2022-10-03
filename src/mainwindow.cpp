/*
 * Copyright (c) 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * See README and LICENSE for more details.
 */

#include "yfel_config.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <qdesktopservices.h>
#include <QFutureWatcher>

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initMainwindowData();
    initMenubar();

    connect(&spi_nand_watcher, &QFutureWatcher<QString>::finished, this, [&]() {
        qDebug() << "SPI NAND Get" + spi_nand_watcher.result();
        ui->chip_spi_nand_lineEdit->setText(spi_nand_watcher.result());
        ui->chip_spi_nand_scan_pushButton->setEnabled(true);
        updateStatusBar(tr("Done."));
    });
}

MainWindow::~MainWindow() {
    delete ui;
    delete chip_op;
}

void MainWindow::initMainwindowData() {
    ui->chip_label_2->setText(tr("NONE"));
    ui->statusbar->showMessage(tr("Ready, Version: ") + PROJECT_GIT_HASH, 5000);
    ui->flashfelflash_comboBox->addItem("SPI NAND");
    ui->flashfelflash_comboBox->addItem("SPI NOR");
}

void MainWindow::initMenubar() {
    // menu exit
    ui->actionExit->setShortcut(QKeySequence::Quit);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exitMenuClicked);

    // menu about
    connect(ui->actionAbout_YFEL, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About YFEL"),
                           tr("Copyright 2022 YuzukiTsuru\n\nGNU General Public License v3.0") + "\n\tVersion: " +
                           PROJECT_GIT_HASH);
    });

    // menu web
    connect(ui->actionWeb, &QAction::triggered, this, []() {
        QString URL = "https://github.com/YuzukiTsuru/YFEL";
        QDesktopServices::openUrl(QUrl(URL.toLatin1()));
    });

    // enable jtag
    connect(ui->actionEnable_JTAG, &QAction::triggered, this, &MainWindow::enableJtag);
    connect(ui->Misc_enable_jtag, &QPushButton::clicked, this, &MainWindow::enableJtag);

    // reset chip
    connect(ui->actionReset_CPU, &QAction::triggered, this, &MainWindow::chipReset);
    connect(ui->Misc_reset_pushButton, &QPushButton::clicked, this, &MainWindow::chipReset);
}

void MainWindow::exitMenuClicked() {
    QMessageBox msgBox;
    msgBox.setText(tr("Exit YFEL?"));
    msgBox.setWindowIcon(QIcon(":/assets/img/icon.png"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Yes) {
        QApplication::quit();
    }
}

void MainWindow::copyToClipboard(const QString &data, QPushButton *button) {
    qDebug() << "Copy " << data << "to Clip Board";
    QClipboard *clip = QApplication::clipboard();
    clip->setText(data);
    button->setText("√");
    QTimer::singleShot(500, this, [button]() { button->setText(tr("Copy")); });
}

void MainWindow::updateStatusBar(const QString &status) {
    ui->statusbar->showMessage(status, 1000);
}

void MainWindow::on_scan_pushButton_clicked() {
    updateStatusBar(tr("Scanning..."));
    try {
        chip_op->chip_scan_chip();
        // Set Scan Button label
        ui->chip_label_2->setText("0x" + QString::number(chip_op->get_current_chip().chip_id, 16));

        // Set CHip lines
        ui->chip_name_lineEdit->setText(chip_op->get_current_chip().chip_name);
        ui->chip_id_lineEdit->setText("0x" + QString::number(chip_op->get_current_chip().chip_id, 16));
        ui->chip_sid_lineEdit->setText("0x" + chip_op->get_current_chip().chip_sid);

        QString chip_core_names_ = chip_op->get_current_chip().chip_core_count_str + " "
                                   + chip_op->get_current_chip().chip_core;
        if (chip_op->get_current_chip().chip_type == chip_type_e::Heterogeneous) {
            for (auto const &item: chip_op->get_current_chip().chip_heterogeneous_core) {
                chip_core_names_.append(" + ");
                chip_core_names_.append(item);
            }
        }
        ui->chip_core_lineEdit->setText(chip_core_names_);

        // update status bar
        updateStatusBar(tr("Done."));
    } catch (const std::exception &e) {
        clearChipInfo();
        QMessageBox::warning(this, tr("Warning"), tr(e.what()));
    }
}

void MainWindow::on_chip_chip_name_pushButton_clicked() {
    copyToClipboard(ui->chip_name_lineEdit->text(), ui->chip_chip_name_pushButton);
}

void MainWindow::on_chip_chip_id_pushButton_2_clicked() {
    copyToClipboard(ui->chip_id_lineEdit->text(), ui->chip_chip_id_pushButton_2);
}

void MainWindow::on_chip_chip_sid_pushButton_3_clicked() {
    copyToClipboard(ui->chip_sid_lineEdit->text(), ui->chip_chip_sid_pushButton_3);
}

void MainWindow::on_chip_chip_core_pushButton_4_clicked() {
    copyToClipboard(ui->chip_core_lineEdit->text(), ui->chip_chip_core_pushButton_4);
}

void MainWindow::on_chip_spi_nor_scan_pushButton_clicked() {
    qDebug() << "Scanning SPI NOR...";
}

void MainWindow::on_chip_spi_nand_scan_pushButton_clicked() {
    qDebug() << "Scanning SPI NAND...";
    updateStatusBar(tr("Scanning SPI NAND..."));
    try {
        auto nand_scan = chip_op->chip_scan_spi_nand();
        spi_nand_watcher.setFuture(nand_scan);
        ui->chip_spi_nand_scan_pushButton->setEnabled(false);
    } catch (const std::runtime_error &e) {
        QMessageBox::warning(this, tr("Warning"), tr(e.what()));
    }
}

void MainWindow::enableJtag() {
    qDebug() << "Enable Chip JTAG";
    try {
        chip_op->chip_enable_jtag();
        QMessageBox::information(this, tr("Info"), tr("JTAG Enabled"));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Warning"), tr(e.what()));
    }
}

void MainWindow::chipReset() {
    qDebug() << "Reset Chip";
    try {
        chip_op->chip_reset_chip();
        clearChipInfo();
        QMessageBox::information(this, tr("Info"), tr("Chip Reseted"));
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Warning"), tr(e.what()));
    }
}

void MainWindow::clearChipInfo() {
    ui->chip_label_2->setText(tr("NONE"));
    ui->chip_id_lineEdit->setText("");
    ui->chip_name_lineEdit->setText("");
    ui->chip_sid_lineEdit->setText("");
    ui->chip_core_lineEdit->setText("");
}

void MainWindow::on_Misc_exec_addr_btn_clicked() {
    // Check whether the address is entered
    if (ui->Misc_exec_addr_lineEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter address."));
        return;
    }
    try {
        // Get the address
        auto addrString = ui->Misc_exec_addr_lineEdit->text();
        bool convertStatus = false;
        uint32_t addr;

        // Check whether the input address is HEX
        if (addrString.startsWith("0x")) {
            // In case of HEX, delete the first two char
            addrString.remove(0, 2);
            addr = static_cast<uint32_t>(addrString.toInt(&convertStatus, 16));
        } else {
            addr = static_cast<uint32_t>(addrString.toInt(&convertStatus));
        }
        // Check whether the address is actually entered
        if (convertStatus) {
            chip_op->chip_exec(addr);
            // After execution, the device will disconnect the link and clear the UI
            clearChipInfo();
            QMessageBox::information(this, tr("Info"), tr("Run command sent, device disconnected"));
        } else {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter the correct address"));
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Warning"), tr(e.what()));
    }
}

