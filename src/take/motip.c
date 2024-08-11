#include "motip.h"
#include <cglm/cglm.h>

void LinerInterPolateMatrix(FMATRIX dm, FMATRIX m0, FMATRIX m1, f32 weight) {
    vec4 rot0;
    vec4 rot1;
    vec4 rot;
    vec4 trans;

    glm_mat4_quat(m0, rot0);
    glm_mat4_quat(m1, rot1);
    glm_quat_slerp(rot0, rot1, weight, rot);
    glm_vec4_lerp(m0[3], m1[3], weight, trans);

    glm_quat_mat4(rot, dm);
    glm_translated(dm, trans);
}
