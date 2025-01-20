#ifndef WINDOW_ACCELERATION_H_
#define WINDOW_ACCELERATION_H_

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

enum window_acceleration {
    WINDOW_ACCELERATION_UNSET = -1,
    WINDOW_ACCELERATION_NONE = 0,
    WINDOW_ACCELERATION_EGL_OPENGL,
    WINDOW_ACCELERATION_VULKAN,
    WINDOW_ACCELERATION_MAX_
};

#endif /* WINDOW_ACCELERATION_H_ */
