#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <vector>
#include <tuple>
#include <memory>

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::vector;
using std::tuple;
using std::tie;
using std::make_tuple;
using std::shared_ptr;

#include "io.h"
#include "matrix.h"

#include "MyObject.h"

typedef Matrix<uint> umatrix;

class bin {
	public:
		static const uint radius = 0;
		static const uint threshold = 50;
		
		uint operator ()(const Image& in) const 
		{
			uint r, g, b, mean, size;
			size = 2 * radius + 1;
			for (uint i = 0; i < size; i++)
				for (uint j = 0; j < size; j++) 
				{
					tie(r, g, b) = in(i, j);
					mean = (r + g + b) / 3;
					if (mean > threshold) 
						return 1;
					else
						return 0;
				}
			return 0;
		}
};

void Fill(const umatrix& img,umatrix& labels,uint x,uint y,long L)
{
	if( (labels(x,y)==0) && (img(x,y)==1))
	{		
		labels(x,y)=L;
		if(x>0)
			Fill(img,labels,x-1,y,L); //left right up down 
		if(x<img.n_rows-1)
			Fill(img,labels,x+1,y,L);
		if(y>0)
			Fill(img,labels,x,y-1,L);
		if(y<img.n_cols-1)
			Fill(img,labels,x,y+1,L);
	}
}
	
	
long Labelling(const umatrix& img,umatrix& labels) 
{
	long L=1;
	for (uint y=0;y<img.n_cols;y++) 
		for(uint x=0;x<img.n_rows;x++) 
			if ((img(x,y)>0) && labels(x,y)==0) 
			{	
				Fill(img,labels,x,y,L++); //current pixel
			}
	L--;
	return L;
}

Image bin_to_bmp(const umatrix bi) 
{
	Image out(bi.n_rows,bi.n_cols);
	for ( uint i = 0; i < out.n_rows; i++) 
		for ( uint j = 0; j < out.n_cols; j++ )
			if ((bi(i,j)==255)) 
				
				out(i,j)=make_tuple(bi(i,j)*1,0,0);
			
			else
				out(i,j)=make_tuple(0,bi(i,j)*255,0);
		
	return out.deep_copy();
}

typedef tuple<int,int> location;
location Local(const umatrix& labmap, long L)
{
	uint xc,yc;
	uint xsum=0,ysum=0;
	uint numb=0;	
	
	for(uint x=0;x<labmap.n_rows;x++) 
		for (uint y=0;y<labmap.n_cols;y++) 
	
		if (labmap(x,y)==L)
		{
			xsum+=x; 
			ysum+=y;
			numb++;
		}
	xc=xsum/numb;
	yc=ysum/numb;
			
	return make_tuple(xc,yc);
}
float max_radius(const umatrix& labmap, long L, location local)
{
	float max=0,r;
	uint i,j;
	
	tie(i,j)=local;
	
	for(uint x=0;x<labmap.n_rows;x++)
		for (uint y=0;y<labmap.n_cols;y++) 
			if (labmap(x,y)==L)
			{	
				r=sqrt((x-i)*(x-i)+(y-j)*(y-j));
				if (max<r) max=r;
			}
	return round(max);
}
float min_radius(const umatrix& labmap, location local)
{
	float min,r;
	uint i,j;
	
	tie(i,j)=local;
	min=labmap.n_cols;
	
	for(uint x=0;x<labmap.n_rows;x++)
		for (uint y=0;y<labmap.n_cols;y++) 	
			if (labmap(x,y)==0)
			{
				r=sqrt((x-i)*(x-i)+(y-j)*(y-j));
				if (min>r) min=r;
			}
	return trunc(min);
}

tuple< vector< shared_ptr<IObject> >, location> 
definition(const Image& in, const umatrix& labmap, long L)
{
	vector<shared_ptr<IObject>> list;
	int numb=1;
	uint r,g,b;
	int maxr, minr;
	uint i,j;
	location alocal; 
	location local;
	while (numb<=L)
	{
		local = Local(labmap,numb);
		tie(i,j) = local;
		tie(r,g,b) = in(i,j);
		if (r>g)  
			{
				alocal = local; 
				shared_ptr <IObject> axis = 
					std:: make_shared <Axis>(make_tuple(j,i));
				list.push_back(axis);
			}
		else 
		{
			maxr = max_radius(labmap,numb,local);
			minr = min_radius(labmap, local);
			shared_ptr <IObject> gear = 
				std:: make_shared <Gear>(make_tuple(j,i),minr,maxr,false);
			list.push_back( gear);
		}
		numb++;
	}
	return make_tuple(list, alocal);
}
tuple<int, vector<shared_ptr<IObject>>, Image>
repair_mechanism(const Image& in, const char* path)
{
    // Base: return array of found objects and index of the correct gear
    umatrix binary_Map(in.n_rows, in.n_cols);
    binary_Map = in.unary_map(bin());
    umatrix Labels(in.n_rows, in.n_cols);
    for(uint i = 0; i < in.n_rows; i++)
		for(uint j = 0; j < in.n_cols; j++)
			Labels(i, j) = 0;
			
	long counter = Labelling(binary_Map, Labels);
	
    // Bonus: return additional parameters of gears
    auto object_array = vector<shared_ptr<IObject>>();
    
    location alocal;
	tie(object_array, alocal) = definition(in, Labels, counter);
	
	 // find pic
   
        int len=strlen(path);
        string path1, path2, path3; 
        for (int i = 0; i < len-4; i++)
			{
				path1.push_back(path[i]);
				path2.push_back(path[i]);
				path3.push_back(path[i]);
			}
			
        path1+="_1.bmp";
        path2+="_2.bmp";
        path3+="_3.bmp";
           
        Image newpic[3];
        newpic[0] = load_image(path1.c_str() );
          newpic[1] = load_image(path2.c_str() );
            newpic[2] = load_image(path3.c_str() );
          
		int pic=1;
		
		Image new_image = in;
		umatrix newlab(in.n_rows, in.n_cols);
		for(uint i = 0; i < in.n_rows; i++)
		for(uint j = 0; j < in.n_cols; j++)
				newlab(i, j) = 0;
				
				
    
    int picAns = 0;
    int cntAns = 0;
	for ( pic=0; pic<=2; pic++)
	{ 
		uint curr;   

		umatrix gear_bin(newpic[pic].n_rows, newpic[pic].n_cols);
		gear_bin = newpic[pic].unary_map(bin());
        int cnt = 0;
        for (uint i = 0; i < gear_bin.n_rows; ++i) {
            for(uint j = 0; j < gear_bin.n_cols; j++) {
                if (gear_bin(i, j)) {
                    ++cnt;
                }
            }
        }
	
		new_image = bin_to_bmp(binary_Map);
		for(uint i = 0; i < in.n_rows; i++)
		for(uint j = 0; j < in.n_cols; j++)
				newlab(i, j) = 0;
		
		int x1,y1;
		tie(x1, y1) = Local(gear_bin,1);
		uint xa,ya;
		tie(xa,ya) = alocal;
	
		int a=0;
		a=xa-x1;
		if (a<0) continue;
		a=ya-y1;
		if (a<0) continue;
		
		for(uint i = 0; i < newpic[pic].n_rows; i++)
			for(uint j = 0; j < newpic[pic].n_cols; j++)
			{
				curr = gear_bin(i, j); 
				if(curr)
				{
					new_image((i + xa - x1), (j + ya - y1)) = 
											make_tuple(255, 0, 0);
					
				}
			
			}
		umatrix newbinary_Map(in.n_rows, in.n_cols);
		newbinary_Map = new_image.unary_map(bin());
		long newn = Labelling(newbinary_Map, newlab);
		
		if (counter==newn ) {
            if (cnt > cntAns) {
                picAns = pic + 1;
                cntAns = cnt;
            }
        }
		
	}
	umatrix newbinary_Map(in.n_rows, in.n_cols);
	
	
    int result_idx = picAns;
    
    Image dst = new_image;
   
    
    return make_tuple(result_idx, object_array, dst);
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        cout << "Usage: " << endl << argv[0]
             << " <in_image.bmp> <out_image.bmp> <out_result.txt>" << endl;
        return 0;
    }

    try {
        Image src_image = load_image(argv[1]);
        ofstream fout(argv[3]);
       

        vector<shared_ptr<IObject>> object_array;
        Image dst_image;
        int result_idx;
        //add path to file 
        tie(result_idx, object_array, dst_image) = 
							repair_mechanism(src_image, argv[1]);

        save_image(dst_image, argv[2]);

        fout << result_idx << endl;
        fout << object_array.size() << endl;
        for (const auto &obj : object_array)
            obj->Write(fout);

    } catch (const string &s) {
        cerr << "Error: " << s << endl;
        return 1;
    }
}
