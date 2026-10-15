! Original FT4/FT2 search order, used only by differential test targets.
subroutine sync4d_search(cd,tweak,fmin,fmax,fstep,tmin,tmax,tstep,ibest,idfbest,smax)
  implicit none
  complex cd(0:23*3456/18-1),tweak(64,-16:16)
  integer fmin,fmax,fstep,tmin,tmax,tstep,ibest,idfbest,f,t
  real smax,sync
  ibest=-1
  idfbest=0
  smax=-99.0
  do f=fmin,fmax,fstep
     do t=tmin,tmax,tstep
        call sync4d(cd,t,tweak(:,f),1,sync)
        if(sync.gt.smax) then
           smax=sync
           ibest=t
           idfbest=f
        endif
     enddo
  enddo
end subroutine sync4d_search
