#include <X11/Xlib.h>
#include <IMdkit.h>
#include <Xi18n.h>

#include "fcitx/module.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include "ximqueue.h"

struct _XimQueue {
    XimCallType type;
    XPointer ptr;
};

static const UT_icd ptr_icd = { sizeof(XimQueue), NULL, NULL, NULL };

void XimQueueInit(FcitxXimFrontend* xim)
{
    utarray_new(xim->queue, &ptr_icd);
}

void XimQueueDestroy(FcitxXimFrontend* xim)
{
    utarray_free(xim->queue);
}

void
XimConsumeQueue(FcitxXimFrontend *xim)
{
    if (!xim->ims)
        return;
    XimQueue *item;

    size_t len = utarray_len(xim->queue);

    for (item = (XimQueue*) utarray_front(xim->queue);
         item != NULL;
         item = (XimQueue*) utarray_next(xim->queue, item)) {

        // modify by caimengci,date:2022/7/12
        // 解决ptr为空时，执行free操作，程序崩溃的问题
        if (item->ptr == NULL) {
            continue;
        }

        switch(item->type) {
        case XCT_FORWARD:
            IMForwardEvent(xim->ims, item->ptr);
            break;
        case XCT_CALLCALLBACK: {
            IMCallCallback(xim->ims, item->ptr);
            IMPreeditCBStruct* pcb = (IMPreeditCBStruct*) item->ptr;
            if (pcb->major_code == XIM_PREEDIT_DRAW) {
                XFree(pcb->todo.draw.text->string.multi_byte);
                free(pcb->todo.draw.text);
            }
            break;
        }
        case XCT_COMMIT: {
            IMCommitString(xim->ims, item->ptr);
            IMCommitStruct *cms = (IMCommitStruct*)item->ptr;
            XFree(cms->commit_string);
            break;
        }
        case XCT_PREEDIT_START:
            IMPreeditStart(xim->ims, item->ptr);
            break;
        case XCT_PREEDIT_END:
            IMPreeditEnd(xim->ims, item->ptr);
            break;
        }

        // modify by caimengci,date:2022/7/12
        // 解决ptr为空时，执行free操作，程序崩溃的问题
        if (item->ptr != NULL) {
            free(item->ptr);
            item->ptr = NULL;
        }
    }

    utarray_clear(xim->queue);
    if (len) {
        FcitxInstanceSetRecheckEvent(xim->owner);
    }
}

void XimPendingCall(FcitxXimFrontend* xim, XimCallType type, XPointer ptr)
{
    // modify by caimengci,date:2022/7/12
    // 解决ptr为空时，执行free操作，程序崩溃的问题
    if (ptr != NULL && xim != NULL) {
        XimQueue item;
        item.type = type;
        item.ptr = ptr;
        utarray_push_back(xim->queue, &item);
    }
}
