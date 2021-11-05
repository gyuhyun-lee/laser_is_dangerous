internal void
OpenGLRectangle(v2 minPos, v2 maxPos, v4 color)
{
   glBegin(GL_TRIANGLES);

    // NOTE : Lower Traingle
   glTexCoord2f(0.0f, 0.0f);
   glVertex2f(minPos.x, minPos.y);

   glTexCoord2f(1.0f, 0.0f);
   glVertex2f(maxPos.x, minPos.y);

   glTexCoord2f(1.0f, 1.0f);
   glVertex2f(maxPos.x, maxPos.y);

    // NOTE : Lower Triangle
   glTexCoord2f(0.0f, 0.0f);
   glVertex2f(minPos.x, minPos.y);

   glTexCoord2f(1.0f, 1.0f);
   glVertex2f(maxPos.x, maxPos.y);

   glTexCoord2f(0.0f, 1.0f);
   glVertex2f(minPos.x, maxPos.y);

   glEnd();
}

internal void
OpenGLRenderGroupToOutputBuffer(render_group *renderGroup, loaded_bitmap *outputTarget, 
                        rect2i clipRect, bool even)
{
    // TODO : Get rid of this!
    real32 nullPixelsToMeters = 0.0f;

    for(uint32 baseAddress = 0;
        baseAddress < renderGroup->pushBufferSize;
        )
    {
        render_group_entry_header *header = (render_group_entry_header *)(renderGroup->pushBufferBase + baseAddress);
        baseAddress += sizeof(*header);
        void *data = (uint8 *)header + sizeof(*header);

        switch(header->type)
        {
            case RenderGroupEntryType_render_group_entry_clear:
            {
                render_group_entry_clear *entry = (render_group_entry_clear *)data;

                glClearColor(entry->color.r, entry->color.g, entry->color.b, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                baseAddress += sizeof(*entry);
            }break;

            case RenderGroupEntryType_render_group_entry_bitmap:
            {
                render_group_entry_bitmap *entry = (render_group_entry_bitmap *)data;

                v2 minPos = entry->pos;
                v2 maxPos = entry->pos + entry->size;

                // TODO : Actual texture mapping here!
                OpenGLRectangle(minPos, maxPos, entry->color);

                baseAddress += sizeof(*entry);
            }break;

            case RenderGroupEntryType_render_group_entry_rectangle:
            {
                render_group_entry_rectangle *entry = (render_group_entry_rectangle *)data;

                v2 minPos = entry->pos;
                v2 maxPos = entry->pos + entry->dim;

                OpenGLRectangle(minPos, maxPos, entry->color);

                // DrawRectangle(outputTarget, entry->pos, entry->pos + entry->dim, entry->color,
                //                 clipRect, even);

                baseAddress += sizeof(*entry);
            }break;

            InvalidDefaultCase;
        }
    }
}
