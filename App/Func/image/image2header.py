import os
import cv2

if __name__ == "__main__":

    list = os.listdir(os.path.dirname(__file__))
    
    for item in list:
        if item.endswith('.png'):
            type = item.rsplit('.', 1)[0]
            header_file = os.path.join(os.path.dirname(__file__), type + '.h')
            img_file = os.path.join(os.path.dirname(__file__), item)
            create_need = False
            if not os.path.exists(header_file):
                create_need = True
            else:
                img_time = os.path.getmtime(img_file)
                header_time = os.path.getmtime(header_file)
                create_need =  img_time > header_time
                
            if not create_need:
                continue
            img = cv2.imread(os.path.join(os.path.dirname(__file__), item), cv2.IMREAD_UNCHANGED)

            height, width = img.shape[:2]

            c_struct_data = '#ifndef _CURSOR_' + type.upper() + '_H_\n'
            c_struct_data += '#define _CURSOR_' + type.upper() + '_H_\n\n'
            c_struct_data += '#define CURSOR_' + type.upper() + '_WIDTH %d' % width + '\n'
            c_struct_data += '#define CURSOR_' + type.upper() + '_HEIGHT %d' % height + '\n\n'
            c_struct_data += 'const static EFI_GRAPHICS_OUTPUT_BLT_PIXEL CursorImage' + type.capitalize() +'[] = {\n'
            
            for y in range(height):
                c_struct_data += '  '
                for x in range(width):
                    pixel = img[y, x]
                    r, g, b, a = pixel
                    c_struct_data += "{0x%02X, 0x%02X, 0x%02X, 0x%02X}," % (r, g, b, a)
                c_struct_data += "\n"
            
            c_struct_data += '};\n\n#endif\n'
            
            with open(header_file, 'w') as f:
                f.write(c_struct_data)
            