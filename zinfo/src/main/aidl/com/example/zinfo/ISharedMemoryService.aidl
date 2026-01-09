package com.example.zinfo;

import android.os.ParcelFileDescriptor;

interface ISharedMemoryService {
    void receiveFd(in ParcelFileDescriptor pfd);
}

