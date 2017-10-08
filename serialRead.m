function [vecx, vec]= serialRead()
close all
s = serial('COM6','BaudRate',9600);
%get(s,{'Type','Name','Port',})
counter=0;

vec=[];
vecx=[];



f = figure;


% Create push button
    btn = uicontrol('Style', 'pushbutton', 'String', 'Start',...
        'Position', [20 20 50 20],...
        'Callback', @toggle);  
    
      plot(0)

while (ishandle(f))
   
 if(strcmp(btn.String,'Stop'))
  
     aux =str2num(fscanf(s));
     vec= [vec,aux(1)];
    
     if(size(aux,2)>1)
     vecx= [vecx,aux(2)];
     plot(vecx,vec);    
     else
     plot(vec);   
     end
     
     
    %pause so you can actually press break;
   
 end
     pause(0.01);
end
 
 
 fclose(s)
delete(s)
clear s


 function toggle(source,event)
      if(strcmp(btn.String,'Start'))
     btn.String ='Stop';
     
     fopen(s)
     
      else
     btn.String ='Start';
     vec=[];
     vecx=[];
     fclose(s)
      end
    end


end


