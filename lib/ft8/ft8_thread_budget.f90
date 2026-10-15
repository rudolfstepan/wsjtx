pure integer function ft8_thread_budget(ncores,nrequested,nlimit) result(n)
  implicit none
  integer, intent(in) :: ncores,nrequested,nlimit
  integer :: cores
  cores=max(1,ncores)
  if(nrequested.ge.1 .and. nrequested.le.12) then
     n=nrequested
  else if(cores.le.4) then
     n=max(1,cores-1)
  else if(cores.le.8) then
     n=cores-2
  else if(cores.le.15) then
     n=cores-3
  else
     n=12
  endif
  ! The FT8 section workers contain barriers. Starting fewer workers than
  ! there are sections can deadlock, so honor OpenMP's actual runtime limits.
  n=max(1,min(n,12,cores,max(1,nlimit)))
end function ft8_thread_budget
