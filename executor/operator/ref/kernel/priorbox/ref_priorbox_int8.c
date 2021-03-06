static int ref_priorbox_int8(int8_t* output, priorbox_ref_param* param, int elem_size)
{
    float* output_f32 = ( float* )malloc(param->out_dim * 2 * sizeof(float));
    int num_priors = param->num_priors;
    float offset = param->offset;
    for(int h = 0; h < param->feature_h; ++h)
    {
        float* box = output_f32 + h * num_priors * 4 * param->feature_w;
        for(int w = 0; w < param->feature_w; ++w)
        {
            float center_x = (w + offset) * param->step_w;
            float center_y = (h + offset) * param->step_h;
            float box_width, box_height;
            for(int s = 0; s < ( int )param->min_size_num; ++s)
            {
                int min_size = param->min_size[s];
                // first prior: aspect_ratio = 1, size = min_size
                box_width = box_height = min_size;
                box[0] = (center_x - box_width * 0.5f) / param->image_w;
                box[1] = (center_y - box_height * 0.5f) / param->image_h;
                box[2] = (center_x + box_width * 0.5f) / param->image_w;
                box[3] = (center_y + box_height * 0.5f) / param->image_h;
                box += 4;

                // default：len(max_size)=len(min_size)
                if(param->max_size_num > 0)
                {
                    int max_size = param->max_size[s];
                    // second prior: aspect_ratio = 1, size = sqrt(min_size * max_size)
                    box_width = box_height = sqrt(min_size * max_size);
                    box[0] = (center_x - box_width * 0.5f) / param->image_w;
                    box[1] = (center_y - box_height * 0.5f) / param->image_h;
                    box[2] = (center_x + box_width * 0.5f) / param->image_w;
                    box[3] = (center_y + box_height * 0.5f) / param->image_h;
                    box += 4;
                }

                // rest of priors
                for(int r = 0; r < ( int )param->aspect_ratio_size; ++r)
                {
                    float ar = param->aspect_ratio[r];
                    box_width = min_size * sqrt(ar);
                    box_height = min_size / sqrt(ar);
                    box[0] = (center_x - box_width * 0.5f) / param->image_w;
                    box[1] = (center_y - box_height * 0.5f) / param->image_h;
                    box[2] = (center_x + box_width * 0.5f) / param->image_w;
                    box[3] = (center_y + box_height * 0.5f) / param->image_h;
                    box += 4;
                    if(param->flip)
                    {
                        box[0] = (center_x - box_height * 0.5f) / param->image_h;
                        box[1] = (center_y - box_width * 0.5f) / param->image_w;
                        box[2] = (center_x + box_height * 0.5f) / param->image_h;
                        box[3] = (center_y + box_width * 0.5f) / param->image_w;
                        box += 4;
                    }
                }
            }
        }
    }
    // clip the prior's coordidate such that it is within [0, 1]
    int dim = param->out_dim;
    if(param->clip)
    {
        for(int d = 0; d < dim; ++d)
        {
            output_f32[d] = std::min(std::max(output_f32[d], 0.f), 1.f);
        }
    }
    // set the variance.
    float* output_ptr = output_f32 + dim;
    int size = dim / 4;
    for(int i = 0; i < size; i++)
    {
        output_ptr[0] = param->variance[0];
        output_ptr[1] = param->variance[1];
        output_ptr[2] = param->variance[2];
        output_ptr[3] = param->variance[3];
        output_ptr += 4;
    }
    // covert fp32 -> fp16
    int out_size = 2 * dim;
    float output_max = fabs(output_f32[0]);

    for(int n = 1; n < out_size; n++)
    {
        if(fabs(output_f32[n]) > output_max)
        {
            output_max = fabs(output_f32[n]);
        }
    }
    float output_scale = output_max / 127;

    for(int i = 0; i < out_size; ++i)
    {
        int output_data = round(output_f32[i] / output_scale);
        if(output_data > 127)
            output[i] = 127;
        else if(output_data < -127)
            output[i] = -127;
        else
            output[i] = ( int8_t )output_data;
    }
    param->out_scale = output_scale;
    param->out_zero = 0;

    free(output_f32);
    output_f32 = NULL;

    return 0;
}
