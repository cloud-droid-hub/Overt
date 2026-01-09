package com.example.overt;

import android.os.ParcelFileDescriptor;

interface ISharedMemoryService {
    void receiveFd(in ParcelFileDescriptor pfd);
}

