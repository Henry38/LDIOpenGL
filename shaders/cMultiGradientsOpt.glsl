#version 430 compatibility
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/structures.hglsl"
#include "/buffers.hglsl"
#include "/uniforms.hglsl"
#include "/auxiliary_functions.hglsl"

layout(local_size_x=1024, local_size_y=1, local_size_z=1) in;
//layout(binding = 0, offset = 0) uniform atomic_uint vAtomicCounter;

const int magnitude = 100000;
const float x_min = float(box[0])/float(magnitude);//-0.2;
const float y_min = float(box[1])/float(magnitude);//-0.2;
const float z_min = float(box[2])/float(magnitude);//-0.2;
const float x_max = float(box[3])/float(magnitude);//+0.2;
const float y_max = float(box[4])/float(magnitude);//+0.2;
const float z_max = float(box[5])/float(magnitude);//+0.2;

void main()
{
    float atomicVol = 0;
    float atomicBaryA_top=0, atomicBaryB_top=0, atomicBaryC_top=0;
    float atomicBaryA_down=0, atomicBaryB_down=0, atomicBaryC_down=0;

    uint id = gl_GlobalInvocationID.x;
    uint start_offset, end_offset;
    start_offset = vPrefixSums[id];
    if(id >= NB_PIXELS)
        return;
    end_offset = vPrefixSums[id+1];
    if(start_offset == end_offset)
        return;
    //collision computation
    uint acc = 0;
    opt_frag topFrag, downFrag;
    for(uint i=start_offset; i<end_offset; i++)
    {
        topFrag = vOptFrags[i];
        uint normal_in = unpack_in(topFrag);
        if(normal_in == 0)
        {
            acc++;
            if(acc == 2)
            {
                downFrag = topFrag;
            }
        }
        if(normal_in == 1)
        {
            if(acc == 2 && (downFrag.info_2.y != topFrag.info_2.y))
            {
                //atomicCounterIncrement(vAtomicCounter);
                //get the cell number of our current fragment, xyz(coords) <=> klm(cell_nb)
                //this computation depends on the render direction
                uint k=0, l=0, m=0, m_down=0, m_top=0;
                float cell_step=0, boxMax=0, boxMin=0;
                float z_minus=0, z_plus=0;
                int nb_steps = 0;
                vec3 topPos = getPos(uint(topFrag.info_2.x), topFrag.info_2.y);
                vec3 downPos = getPos(uint(downFrag.info_2.x), downFrag.info_2.y);
                if(RENDER_DIR == 2) //rendering along the z-axis
                {
                    z_plus = topPos.z;
                    z_minus = downPos.z;
                    boxMax = z_max;
                    boxMin = z_min;
                    float cell_step_x = (x_max-x_min)/float(NB_DIVIDE);
                    float cell_step_y = (y_max-y_min)/float(NB_DIVIDE);
                } else if(RENDER_DIR == 1) //rendering along the y-axis
                {
                    z_plus = topPos.y;
                    z_minus = downPos.y;
                    boxMax = y_max;
                    boxMin = y_min;
                    float cell_step_x = (x_max-x_min)/float(NB_DIVIDE);
                    float cell_step_z = (z_max-z_min)/float(NB_DIVIDE);
                } else { //rendering along the x-axis
                    z_plus = topPos.x;
                    z_minus = downPos.x;
                    boxMax = x_max;
                    boxMin = x_min;
                    float cell_step_z = (z_max-z_min)/float(NB_DIVIDE);
                    float cell_step_y = (y_max-y_min)/float(NB_DIVIDE);
                }
                float cell_step_x = (x_max-x_min)/float(NB_DIVIDE);
                float cell_step_z = (z_max-z_min)/float(NB_DIVIDE);
                float cell_step_y = (y_max-y_min)/float(NB_DIVIDE);
                k = uint(floor((downPos.x-x_min)/cell_step_x));
                l = uint(floor((downPos.y-y_min)/cell_step_y));
                m = uint(floor((downPos.z-z_min)/cell_step_z));
                //s'il s'agit de topFrag.world_x = x_max p.ex.
                //la boîte englobante est très juste
                if(k >= NB_DIVIDE)
                    k = NB_DIVIDE-1;
                if(l >= NB_DIVIDE)
                    l = NB_DIVIDE-1;
                if(m >= NB_DIVIDE)
                    m = NB_DIVIDE-1;
                cell_step = (boxMax - boxMin)/float(NB_DIVIDE); //the subdivision step
                if(z_minus >= boxMin)
                {
                    m_down = uint(floor((z_minus-boxMin)/cell_step));
                    m_top = uint(floor((z_plus-boxMin)/cell_step));
                    nb_steps = int(m_top-m_down)-1;
                }
                ////we start the process at W_minus of the cell m_down and we finish at
                ////W_plus of the cell m_top
                float W_minus = m_down*cell_step + boxMin;
                float W_plus = W_minus + cell_step;
                float alpha, beta;
                int i=0;
                uint NB_VOLUME = NB_DIVIDE*NB_DIVIDE*NB_DIVIDE;
                uint renderStep = RENDER_DIR*NB_VOLUME*(NB_VERTICES+1);
                //From the bottom to the top
                uint cell_nb = NB_DIVIDE*NB_DIVIDE*k + NB_DIVIDE*l + m;
                float downFrag_coordBary_1 = 1.0f-downFrag.info_2.z-downFrag.info_2.w;
                float topFrag_coordBary_1 = 1.0f-topFrag.info_2.z-topFrag.info_2.w;
                alpha = (W_plus-z_minus)/(z_plus-z_minus);
                beta = (W_minus-z_minus)/(z_plus-z_minus);
                float elementaryGradientTop = AREA*(-beta*downFrag_coordBary_1);
                float elementaryGradientDown = AREA*( (1-alpha)*topFrag_coordBary_1 - (1-beta)*topFrag_coordBary_1 );
                float factor_top = -alpha;
                float factor_down = alpha;
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.y + 1)], factor_down*AREA*downFrag_coordBary_1);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.z + 1)], factor_down*AREA*downFrag.info_2.z);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.w + 1)], factor_down*AREA*downFrag.info_2.w);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.y + 1)], factor_top*AREA*topFrag_coordBary_1);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.z + 1)], factor_top*AREA*topFrag.info_2.z);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.w + 1)], factor_top*AREA*topFrag.info_2.w);
                float elementaryVolume = AREA*(W_plus-z_minus);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb], elementaryVolume);

                    W_minus = W_plus;
                    W_plus += cell_step;
                    i++;

                while(i<30) //always loop over a constant for better performance
                {
                    if(i >= nb_steps)
                        break;
                    if(RENDER_DIR == 0){
                        cell_nb = NB_DIVIDE*NB_DIVIDE*(k+(i-1)) + NB_DIVIDE*l + m;
                    } else if(RENDER_DIR == 1) {
                        cell_nb = NB_DIVIDE*NB_DIVIDE*k + NB_DIVIDE*(l+(i-1)) + m;
                    } else {
                        cell_nb = NB_DIVIDE*NB_DIVIDE*k + NB_DIVIDE*l + m + i-1;
                    }
                    alpha = (W_plus-z_minus)/(z_plus-z_minus);
                    beta = (W_minus-z_minus)/(z_plus-z_minus);

                    elementaryVolume = AREA*(W_plus-W_minus);
                    atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb], elementaryVolume);
                    {
                        factor_top = -(alpha - beta);
                        factor_down = -((1-alpha) - (1-beta));
                        //first vertex
                        elementaryGradientTop = AREA*topFrag_coordBary_1*factor_top;
                        elementaryGradientDown = AREA*downFrag_coordBary_1*factor_down;
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.y + 1)], elementaryGradientTop);
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.y + 1)], elementaryGradientDown);
                        //second vertex
                        elementaryGradientTop = AREA*topFrag.info_2.z*factor_top;
                        elementaryGradientDown = AREA*downFrag.info_2.z*factor_down;
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.z + 1)], elementaryGradientTop);
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.z + 1)], elementaryGradientDown);
                        //third vertex
                        elementaryGradientTop = AREA*topFrag.info_2.w*factor_top;
                        elementaryGradientDown = AREA*downFrag.info_2.w*factor_down;
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.w + 1)], elementaryGradientTop);
                        atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.w + 1)], elementaryGradientDown);
                    }

                    //next iteration preparation
                    W_minus = W_plus;
                    W_plus += cell_step;
                    i++;
                }
                    if(RENDER_DIR == 0){
                        cell_nb = NB_DIVIDE*NB_DIVIDE*(k+(i-1)) + NB_DIVIDE*l + m;
                    } else if(RENDER_DIR == 1) {
                        cell_nb = NB_DIVIDE*NB_DIVIDE*k + NB_DIVIDE*(l+(i-1)) + m;
                    } else {
                        cell_nb = NB_DIVIDE*NB_DIVIDE*k + NB_DIVIDE*l + m + i-1;
                    }

                    elementaryVolume = AREA*(z_plus-W_minus);
                    atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb], elementaryVolume);
                    alpha = (W_plus-z_minus)/(z_plus-z_minus);
                    beta = (W_minus-z_minus)/(z_plus-z_minus);
                    factor_top = -(1-beta);
                    factor_down = (1-beta);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.y + 1)], factor_down*AREA*downFrag_coordBary_1);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.z + 1)], factor_down*AREA*downFrag.info_2.z);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(downFrag.info_1.w + 1)], factor_down*AREA*downFrag.info_2.w);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.y + 1)], factor_top*AREA*topFrag_coordBary_1);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.z + 1)], factor_top*AREA*topFrag.info_2.z);
                atomicAdd(vGradients[renderStep+(NB_VERTICES+1)*cell_nb+(topFrag.info_1.w + 1)], factor_top*AREA*topFrag.info_2.w);
            }
            acc -= 1;
        }
    }
}
