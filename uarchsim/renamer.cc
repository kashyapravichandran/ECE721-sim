#include<inttypes.h>
#include<math.h>
#include "renamer.h"
#include <assert.h>
#include<iostream>

using namespace std;

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs, uint64_t n_branches)
{
	
	assert(n_phys_regs>n_log_regs);
	logical_size=n_log_regs;
	physical_size=n_phys_regs;
	size_list=physical_size-logical_size;
		
	assert(n_branches<64);
	branches_size=n_branches;
	
	RMT=new uint64_t [logical_size];
	AMT=new uint64_t [logical_size];
	physical_file=new reg_file [physical_size];
	
	for(int i=0;i<logical_size;i++)	
	{
		RMT[i]=i;
		AMT[i]=i;
	}
	
	for(int i=0;i<physical_size;i++)
	{
		physical_file[i].ready=true;
	}
	
	active_head=active_tail=-1;
	free.list=new uint64_t [size_list];
	active_list=new structure2 [size_list];
			
	for(int i=logical_size;i<physical_size;i++)
	{
		free.list[i-logical_size]=i;
		//cout<<"Free List element "<<i-logical_size<<": "<<free.list[i-logical_size];
	}
	
	free.head=0;
	free.tail=size_list-1;
	
	GBM=0;
	
	branch_checkpoint=new checkpoints[n_branches];
	for(int i=0;i<branches_size;i++)
		branch_checkpoint[i].MT = new uint64_t [logical_size];
	
		
}

bool renamer::stall_reg(uint64_t bundle_dst)
{
	int size=0;
	
	if(free.head==-1)
		size=0;
	else if(free.head<free.tail)
			size=free.tail-free.head+1;
		 else
		 {
			size=size_list-(free.head-free.tail-1);	
		 }	
			
	if((size)>=bundle_dst)
		return false;
	
	return true;
	
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
	uint64_t flag=0, n=GBM;
	for(int i=0;i<branches_size;i++)
	{
		if(n&1)
			flag++;
		n=n>>1;
		
	}
	if(bundle_branch>(branches_size-flag))
		return true;
		
	return false;
}

uint64_t renamer::get_branch_mask()
{
	return GBM;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{
	return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg) // pop free list
{
	// pop an element out of the queue
	// return the element
	assert(free.head!=-1);
	int pos=free.head;
	if(free.head==free.tail)
	{
		free.head=free.tail=-1;
	}
	else
	{
		if(free.head==(size_list-1))
			free.head=0;
		else 
			free.head++;
	}
	
	RMT[log_reg]=free.list[pos];
	//cout<<"Renamed Register : "<<free.list[pos];
	return free.list[pos];
}

uint64_t renamer::checkpoint()
{
	// find a free bit
	// copy rmt to checkpoint mt 
	// copy head 
	// copy GBM s
	
	uint64_t n=GBM;
	int ID;
	for(int i=0;i<branches_size;i++)
	{
		if(!(n&1))
		{
			ID=i;
			break;	
		}
		n=n>>1;
	}
	
	assert(ID<branches_size);
	copy_state(branch_checkpoint[ID].MT,RMT);
	branch_checkpoint[ID].head=free.head;
	branch_checkpoint[ID].recover_GBM=GBM;
	
	GBM|=1<<ID;
	
	return ID;	
}

bool renamer::stall_dispatch(uint64_t bundle_inst)
{
	int size=0;
	if(active_head==-1)
		size=0;
	else if(active_head<active_tail)
		size=active_tail-active_head+1;
	else
	{
		size=size_list-(active_head-active_tail-1);	
	}	
	
	if((size_list-size)>=bundle_inst)
		return false;
	
	return true;
	
}

uint64_t renamer::dispatch_inst(bool dest_valid, uint64_t log_reg, uint64_t phys_reg, bool load, bool store, bool branch, bool amo, bool csr, uint64_t PC) // Push active list
{
	//cout<<"\n Active Tail : "<<active_tail;
 	//cout<<"\n Active Head : "<<active_head;	
	//cout<<"\n Size List   : "<<size_list;
	assert(!(active_head==0&&active_tail==size_list-1)||(active_head==active_tail+1));
	
	if(active_tail==-1)
	{
		active_head=0;
		active_tail=0;
	}
	else 
	{
		if(active_tail==size_list-1)
			active_tail=0;
		else 
			active_tail++;
	}
	
	active_list[active_tail].destination_flag=dest_valid;
	if(active_list[active_tail].destination_flag)
	{
		active_list[active_tail].physical=phys_reg;
		active_list[active_tail].logical=log_reg;
	}
	active_list[active_tail].branch_flag=branch;
	active_list[active_tail].amo_flag=amo;
	active_list[active_tail].load_flag=load;
	active_list[active_tail].store_flag=store;
	active_list[active_tail].csr_flag=csr;
	active_list[active_tail].PC=PC;
	active_list[active_tail].branch_misprediction=false;
	active_list[active_tail].load_violation=false;
	active_list[active_tail].exception=false;
	active_list[active_tail].completed=false;
	active_list[active_tail].value_misprediction=false;
	return active_tail;
}

bool renamer::is_ready(uint64_t phys_reg)
{
	return physical_file[phys_reg].ready;
}

void renamer::clear_ready(uint64_t phys_reg)
{
	physical_file[phys_reg].ready=false;
}

void renamer::set_ready(uint64_t phys_reg)
{
	physical_file[phys_reg].ready=true;
}

uint64_t renamer::read(uint64_t phys_reg)
{
	return physical_file[phys_reg].value;
}

void renamer::write(uint64_t phys_reg, uint64_t value)
{
	physical_file[phys_reg].value=value;
	//physical_file[phys_reg].ready=true;
}

void renamer::set_complete(uint64_t AL_index)
{
	active_list[AL_index].completed=true;
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct)
{
	if(!correct)
	{
		// Need to work on this! 
		active_tail=AL_index;
		/*if(active_tail==-1)
		{
			active_tail=size_list-1;
		}*/
		free.head=branch_checkpoint[branch_ID].head;
		GBM=branch_checkpoint[branch_ID].recover_GBM;
		copy_state(RMT, branch_checkpoint[branch_ID].MT);
	}
	else
	{
		uint64_t temp=1<<branch_ID;
		temp=~temp;
		GBM&=temp;
		for(int i=0;i<branches_size;i++)
		{
			branch_checkpoint[i].recover_GBM&=temp;
		}
	}
}

bool renamer::precommit(bool &completed, bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,bool &load, bool &store, bool &branch, bool &amo, bool &csr,uint64_t &PC)
{
	if(active_head==-1)
		return false;
	else
	{
		completed=active_list[active_head].completed;
		exception=active_list[active_head].exception;
		load_viol=active_list[active_head].load_violation;
		br_misp=active_list[active_head].branch_misprediction;
		val_misp=active_list[active_head].value_misprediction;
		load=active_list[active_head].load_flag;
		store=active_list[active_head].store_flag;
		branch=active_list[active_head].branch_flag;
		amo=active_list[active_head].amo_flag;
		csr=active_list[active_head].csr_flag;
		PC=active_list[active_head].PC;
		return true;
	}
}

void renamer::commit() // pop active list and push free 
{
	assert(active_head!=-1);
	assert(active_list[active_head].completed==true);
	assert(active_list[active_head].exception==false);
	assert(active_list[active_head].load_violation==false);
	
	
	// see if destination is valid
	if(active_list[active_head].destination_flag)
	{
		uint64_t log=active_list[active_head].logical;
		uint64_t push=AMT[log];
		AMT[log]=active_list[active_head].physical;
		
		if(free.tail==-1)
			free.tail=free.head=0;
		else if(free.tail==size_list-1)
				free.tail=0;
			else 
				free.tail++;
		free.list[free.tail]=push;
	//cout<<"\n Destination Register : "<<active_list[active_head].physical<<" Logical "<<active_list[active_head].logical<<" Pushing Value : "<<free.list[free.tail]<<" RMT "<<RMT[log];
	}
	
	// Editing active list 
	//cout<<"\n Destination Register : " <<active_list[active_head].physical<<" Logical "<<active_list[active_head].logical;	
	if(active_head==active_tail)
		active_head=active_tail=-1;
	else if(active_head==size_list-1)
			active_head=0;
		else 
			active_head++;
}

void renamer::squash()
{
	active_head=active_tail=-1;
	free.tail=free.head-1;
	
	if(free.tail==-1)
		free.tail=size_list-1;
		
	copy_state(RMT, AMT);
	
	GBM=0;
	
	for(int i=0;i<physical_size;i++)
	{
		physical_file[i].ready=true;
	}
}

void renamer::set_exception(uint64_t AL_index)
{
	active_list[AL_index].exception=true;
}

void renamer::set_load_violation(uint64_t AL_index)
{
	active_list[AL_index].load_violation=true;
}

void renamer::set_branch_misprediction(uint64_t AL_index)
{
	active_list[AL_index].branch_misprediction=true;
}

void renamer::set_value_misprediction(uint64_t AL_index)
{
	active_list[AL_index].value_misprediction=true;
}

bool renamer::get_exception(uint64_t AL_index)
{
	return active_list[AL_index].exception;
}
