//****************************************************************************
//*  Copyright: National ICT Australia,  2007 - 2011                         *
//*  Developed at the ATP lab, Networked Systems research theme              *
//*  Author(s): Yuriy Tselishchev, Athanassios Boulis                        *
//*  This file is distributed under the terms in the attached LICENSE file.  *
//*  If you do not find this file, copies can be found by writing to:        *
//*                                                                          *
//*      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
//*      Attention:  License Inquiry.                                        *
//*                                                                          *  
//****************************************************************************/

import node.communication.mac.MacPacket;

// Sửa đổi 1: Thêm các loại frame mới và dùng tên enum nhất quán
enum TunableMacFrameType {
    DATA_FRAME = 0;
    BEACON_FRAME = 1;   // Giữ lại để không gây lỗi, dù không còn được sử dụng tích cực
    RTS_FRAME = 2;      // Thêm mới
    CTS_FRAME = 3;      // Thêm mới
    ACK_FRAME = 4;      // Thêm mới
}; // Sửa đổi 2: Thêm dấu chấm phẩy (;) để kết thúc khai báo enum

packet TunableMacPacket extends MacPacket {
    int frameType @enum(TunableMacFrameType); // Sửa đổi 3: Dùng cú pháp @enum và thêm dấu chấm phẩy (;)
}

