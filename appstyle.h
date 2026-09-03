#pragma once

#include <QString>

inline QString appStyleSheet()
{
    return QStringLiteral(R"(
        QMainWindow, QDialog {
            background: #f4f5f7;
            color: #111827;
            font-size: 13px;
        }
        QWidget {
            font-family: "Segoe UI", "Helvetica Neue", sans-serif;
        }
        QLabel#labelPageTitle {
            font-size: 26px;
            font-weight: 700;
            color: #111827;
        }
        QLabel#labelPageSubtitle,
        QLabel#labelGlobalHint,
        QLabel#labelHint,
        QLabel#fieldHint {
            color: #6b7280;
            font-size: 12px;
        }
        QLabel#labelGlobalTitle,
        QLabel#labelItems,
        QLabel#fieldLabel,
        QLabel#dialogTitle {
            font-size: 15px;
            font-weight: 600;
            color: #111827;
        }
        QLabel#dialogSubtitle {
            color: #6b7280;
            font-size: 12px;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background: transparent;
        }
        QWidget#itemCard {
            background: #ffffff;
            border-radius: 12px;
        }
        QWidget#itemCard:hover {
            background: #eef2ff;
        }
        QLabel#itemIndex {
            background: #111827;
            color: #ffffff;
            border-radius: 10px;
            font-size: 11px;
            font-weight: 600;
        }
        QLabel#itemTitle {
            font-size: 14px;
            font-weight: 600;
            color: #111827;
        }
        QLabel#itemPreview {
            color: #6b7280;
            font-size: 12px;
        }
        QLabel#statusOk {
            color: #047857;
            background: #d1fae5;
            border-radius: 10px;
            padding: 3px 10px;
            font-size: 11px;
            font-weight: 600;
        }
        QLabel#statusFail {
            color: #b91c1c;
            background: #fee2e2;
            border-radius: 10px;
            padding: 3px 10px;
            font-size: 11px;
            font-weight: 600;
        }
        QLineEdit, QPlainTextEdit, QKeySequenceEdit {
            background: #ffffff;
            border: 1px solid #e5e7eb;
            border-radius: 10px;
            padding: 8px 12px;
            selection-background-color: #111827;
            selection-color: #ffffff;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QKeySequenceEdit:focus {
            border: 1px solid #111827;
        }
        QPushButton {
            background: #ffffff;
            border: 1px solid #e5e7eb;
            border-radius: 10px;
            padding: 8px 14px;
            color: #111827;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #f3f4f6;
        }
        QPushButton:pressed {
            background: #e5e7eb;
        }
        QPushButton#btnAddItem, QPushButton#btnSave {
            background: #111827;
            color: #ffffff;
            border: none;
            min-height: 40px;
        }
        QPushButton#btnAddItem:hover, QPushButton#btnSave:hover {
            background: #1f2937;
        }
        QPushButton#btnGhost, QPushButton#btnClearGlobal, QPushButton#btnCancel {
            background: transparent;
            border: none;
            color: #4b5563;
        }
        QPushButton#btnGhost:hover, QPushButton#btnClearGlobal:hover, QPushButton#btnCancel:hover {
            background: #e5e7eb;
            color: #111827;
        }
        QPushButton#btnDanger {
            background: transparent;
            border: none;
            color: #b91c1c;
        }
        QPushButton#btnDanger:hover {
            background: #fee2e2;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #d1d5db;
            border-radius: 4px;
            min-height: 24px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");
}
